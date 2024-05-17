#include "aws_secret.hpp"

#include "duckdb/common/case_insensitive_map.hpp"
#include "duckdb/main/extension_util.hpp"

#include <aws/core/Aws.h>
#include <aws/core/auth/AWSCredentialsProviderChain.h>
#include <aws/core/auth/SSOCredentialsProvider.h>
#include <aws/core/auth/STSCredentialsProvider.h>
#include <aws/core/client/ClientConfiguration.h>

namespace duckdb {

//! Parse and set the remaining options
static void ParseCoreS3Config(CreateSecretInput &input, KeyValueSecret &secret) {
	vector<string> options = {"key_id",    "secret",        "region",
	                          "endpoint",  "session_token", "endpoint",
	                          "url_style", "use_ssl",       "s3_url_compatibility_mode"};
	for (const auto &val : options) {
		auto set_region_param = input.options.find(val);
		if (set_region_param != input.options.end()) {
			secret.secret_map[val] = set_region_param->second;
		}
	}
}

//! This constructs the base S3 Type secret
static unique_ptr<KeyValueSecret> ConstructBaseS3Secret(vector<string> &prefix_paths_p, string &type, string &provider,
                                                        string &name) {
	auto return_value = make_uniq<KeyValueSecret>(prefix_paths_p, type, provider, name);
	return_value->redact_keys = {"secret", "session_token"};
	return return_value;
}

//! Generate a custom credential provider chain for authentication
class DuckDBCustomAWSCredentialsProviderChain : public Aws::Auth::AWSCredentialsProviderChain {
public:
	explicit DuckDBCustomAWSCredentialsProviderChain(const string &credential_chain, const string &profile = "") {
		auto chain_list = StringUtil::Split(credential_chain, ';');

		for (const auto &item : chain_list) {
			if (item == "sts") {
				AddProvider(std::make_shared<Aws::Auth::STSAssumeRoleWebIdentityCredentialsProvider>());
			} else if (item == "sso") {
				if (profile.empty()) {
					AddProvider(std::make_shared<Aws::Auth::SSOCredentialsProvider>());
				} else {
					AddProvider(std::make_shared<Aws::Auth::SSOCredentialsProvider>(profile));
				}
			} else if (item == "env") {
				AddProvider(std::make_shared<Aws::Auth::EnvironmentAWSCredentialsProvider>());
			} else if (item == "instance") {
				AddProvider(std::make_shared<Aws::Auth::InstanceProfileCredentialsProvider>());
			} else if (item == "process") {
				AddProvider(std::make_shared<Aws::Auth::ProcessCredentialsProvider>());
			} else if (item == "config") {
				if (profile.empty()) {
					AddProvider(std::make_shared<Aws::Auth::ProfileConfigFileAWSCredentialsProvider>());
				} else {
					AddProvider(std::make_shared<Aws::Auth::ProfileConfigFileAWSCredentialsProvider>(profile.c_str()));
				}
			} else {
				throw InvalidInputException("Unknown provider found while parsing AWS credential chain string: '%s'",
				                            item);
			}
		}
	}
};

static string TryGetStringParam(CreateSecretInput &input, const string &param_name) {
	auto param_lookup = input.options.find(param_name);
	if (param_lookup != input.options.end()) {
		return param_lookup->second.ToString();
	} else {
		return "";
	}
}

//! This is the actual callback function
static unique_ptr<BaseSecret> CreateAWSSecretFromCredentialChain(ClientContext &context, CreateSecretInput &input) {
	Aws::SDKOptions options;
	Aws::InitAPI(options);
	Aws::Auth::AWSCredentials credentials;

	string profile = TryGetStringParam(input, "profile");

	if (input.options.find("chain") != input.options.end()) {
		string chain = TryGetStringParam(input, "chain");

		DuckDBCustomAWSCredentialsProviderChain provider(chain, profile);
		credentials = provider.GetAWSCredentials();
	} else {
		if (input.options.find("profile") != input.options.end()) {
			Aws::Auth::ProfileConfigFileAWSCredentialsProvider provider(profile.c_str());
			credentials = provider.GetAWSCredentials();
		} else {
			Aws::Auth::DefaultAWSCredentialsProviderChain provider;
			credentials = provider.GetAWSCredentials();
		}
	}

	//! If the profile is set we specify a specific profile
	auto s3_config = Aws::Client::ClientConfiguration(profile.c_str());
	auto region = s3_config.region;

	// TODO: We would also like to get the endpoint here, but it's currently not supported byq the AWS SDK:
	// 		 https://github.com/aws/aws-sdk-cpp/issues/2587

	auto scope = input.scope;
	if (scope.empty()) {
		if (input.type == "s3") {
			scope.push_back("s3://");
			scope.push_back("s3n://");
			scope.push_back("s3a://");
		} else if (input.type == "r2") {
			scope.push_back("r2://");
		} else if (input.type == "gcs") {
			scope.push_back("gcs://");
			scope.push_back("gs://");
		} else {
			throw InternalException("Unknown secret type found in aws extension: '%s'", input.type);
		}
	}

	auto result = ConstructBaseS3Secret(scope, input.type, input.provider, input.name);


	if (!region.empty()) {
		result->secret_map["region"] = region;
	}

	AwsSetCredentialsResult ret;
	if (!credentials.IsExpiredOrEmpty()) {
		result->secret_map["key_id"] = Value(credentials.GetAWSAccessKeyId());
		result->secret_map["secret"] = Value(credentials.GetAWSSecretKey());
		result->secret_map["session_token"] = Value(credentials.GetSessionToken());
	}

	Aws::ShutdownAPI(options);

	ParseCoreS3Config(input, *result);

	// Set endpoint defaults TODO: move to consumer side of secret
	auto endpoint_lu = result->secret_map.find("endpoint");
	if (endpoint_lu ==  result->secret_map.end() || endpoint_lu->second.ToString().empty()) {
		if (input.type == "s3") {
			result->secret_map["endpoint"] = "s3.amazonaws.com";
		} else if (input.type == "r2") {
			if (input.options.find("account_id") != input.options.end()) {
				result->secret_map["endpoint"] = input.options["account_id"].ToString() + ".r2.cloudflarestorage.com";
			}
		} else if (input.type == "gcs") {
			result->secret_map["endpoint"] = "storage.googleapis.com";
		} else {
			throw InternalException("Unknown secret type found in httpfs extension: '%s'", input.type);
		}
	}

	// Set endpoint defaults TODO: move to consumer side of secret
	auto url_style_lu = result->secret_map.find("url_style");
	if (url_style_lu ==  result->secret_map.end() || endpoint_lu->second.ToString().empty()) {
		if (input.type == "gcs" || input.type == "r2") {
			result->secret_map["url_style"] = "path";
		}
	}

	return result;
}

void CreateAwsSecretFunctions::Register(DatabaseInstance &instance) {
	vector<string> types = {"s3", "r2", "gcs"};

	for (const auto& type : types) {
		// Register the credential_chain secret provider
		CreateSecretFunction cred_chain_function = {type, "credential_chain", CreateAWSSecretFromCredentialChain};

		// Params for adding / overriding settings to the automatically fetched ones
		cred_chain_function.named_parameters["key_id"] = LogicalType::VARCHAR;
		cred_chain_function.named_parameters["secret"] = LogicalType::VARCHAR;
		cred_chain_function.named_parameters["region"] = LogicalType::VARCHAR;
		cred_chain_function.named_parameters["session_token"] = LogicalType::VARCHAR;
		cred_chain_function.named_parameters["endpoint"] = LogicalType::VARCHAR;
		cred_chain_function.named_parameters["url_style"] = LogicalType::VARCHAR;
		cred_chain_function.named_parameters["use_ssl"] = LogicalType::BOOLEAN;
		cred_chain_function.named_parameters["url_compatibility_mode"] = LogicalType::BOOLEAN;

		if (type == "r2") {
			cred_chain_function.named_parameters["account_id"] = LogicalType::VARCHAR;
		}

		// Param for configuring the chain that is used
		cred_chain_function.named_parameters["chain"] = LogicalType::VARCHAR;

		// Params for configuring the credential loading
		cred_chain_function.named_parameters["profile"] = LogicalType::VARCHAR;

		ExtensionUtil::RegisterFunction(instance, cred_chain_function);
	}
}

} // namespace duckdb
