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
	return_value->redact_keys = {"secret_access_key", "session_token"};
	return return_value;
}

//! Generate a custom credential provider chain for authentication
class DuckDBCustomAWSCredentialsProviderChain : public Aws::Auth::AWSCredentialsProviderChain {
public:
	explicit DuckDBCustomAWSCredentialsProviderChain(const string &credential_chain, const string &profile = "",
	                                                 const string &task_role_resource_path = "",
	                                                 const string &task_role_endpoint = "",
	                                                 const string &task_role_token = "") {
		auto chain_list = StringUtil::Split(credential_chain, ';');

		for (const auto &item : chain_list) {
			if (item == "sts") {
				AddProvider(make_shared<Aws::Auth::STSAssumeRoleWebIdentityCredentialsProvider>());
			} else if (item == "sso") {
				if (profile.empty()) {
					AddProvider(make_shared<Aws::Auth::SSOCredentialsProvider>());
				} else {
					AddProvider(make_shared<Aws::Auth::SSOCredentialsProvider>(profile));
				}
			} else if (item == "env") {
				AddProvider(make_shared<Aws::Auth::EnvironmentAWSCredentialsProvider>());
			} else if (item == "instance") {
				AddProvider(make_shared<Aws::Auth::InstanceProfileCredentialsProvider>());
			} else if (item == "process") {
				AddProvider(make_shared<Aws::Auth::ProcessCredentialsProvider>());
			} else if (item == "task_role") {
				if (!task_role_resource_path.empty()) {
					AddProvider(make_shared<Aws::Auth::TaskRoleCredentialsProvider>(task_role_resource_path.c_str()));
				} else if (!task_role_endpoint.empty()) {
					AddProvider(make_shared<Aws::Auth::TaskRoleCredentialsProvider>(task_role_endpoint.c_str(),
					                                                                task_role_token.c_str()));
				} else {
					throw InvalidInputException(
					    "task_role provider selected without a resource path or endpoint specified!");
				}
			} else if (item == "config") {
				if (profile.empty()) {
					AddProvider(make_shared<Aws::Auth::ProfileConfigFileAWSCredentialsProvider>());
				} else {
					AddProvider(make_shared<Aws::Auth::ProfileConfigFileAWSCredentialsProvider>(profile.c_str()));
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
		string task_role_resource_path = TryGetStringParam(input, "task_role_resource_path");
		string task_role_endpoint = TryGetStringParam(input, "task_role_endpoint");
		string task_role_token = TryGetStringParam(input, "task_role_token");

		DuckDBCustomAWSCredentialsProviderChain provider(chain, profile, task_role_resource_path, task_role_endpoint,
		                                                 task_role_token);
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

	auto result = ConstructBaseS3Secret(input.scope, input.type, input.provider, input.name);

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

	return result;
}

void CreateAwsSecretFunctions::Register(DatabaseInstance &instance) {
	string type = "S3";

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
	cred_chain_function.named_parameters["task_role_resource_path"] = LogicalType::VARCHAR;
	cred_chain_function.named_parameters["task_role_endpoint"] = LogicalType::VARCHAR;
	cred_chain_function.named_parameters["task_role_token"] = LogicalType::VARCHAR;

	ExtensionUtil::RegisterFunction(instance, cred_chain_function);
}

} // namespace duckdb