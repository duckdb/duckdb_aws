#include "aws_secret.hpp"

#include "duckdb/common/case_insensitive_map.hpp"
#include "duckdb/main/extension_util.hpp"

#include <aws/core/Aws.h>
#include <aws/core/auth/AWSCredentialsProviderChain.h>
#include <aws/core/client/ClientConfiguration.h>

namespace duckdb {

//! Parse and set the remaining options
static void ParseCoreS3Config(CreateSecretInput &input, KeyValueSecret &secret) {
	vector<string> options = {
	    "region", "endpoint", "session_token", "endpoint", "url_style", "use_ssl", "s3_url_compatibility_mode"};
	for (const auto &val : options) {
		auto set_region_param = input.options.find(val);
		if (set_region_param != input.options.end()) {
			secret.secret_map[val] = set_region_param->second.ToString();
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

//! This is the actual callback function
static unique_ptr<BaseSecret> CreateAWSSecretFromCredentialChain(ClientContext &context, CreateSecretInput &input) {
	Aws::SDKOptions options;
	Aws::InitAPI(options);
	Aws::Auth::AWSCredentials credentials;

	//! If the profile is set we specify a specific profile
	auto profile_param = input.options.find("profile");
	string profile_string;
	if (profile_param != input.options.end()) {
		profile_string = profile_param->second.ToString();
		Aws::Auth::ProfileConfigFileAWSCredentialsProvider provider(profile_string.c_str());
		credentials = provider.GetAWSCredentials();
	} else {
		Aws::Auth::DefaultAWSCredentialsProviderChain provider;
		credentials = provider.GetAWSCredentials();
	}

	auto s3_config = Aws::Client::ClientConfiguration(profile_string.c_str());
	auto region = s3_config.region;

	// TODO: We would also like to get the endpoint here, but it's currently not supported byq the AWS SDK:
	// 		 https://github.com/aws/aws-sdk-cpp/issues/2587

	auto result = ConstructBaseS3Secret(input.scope, input.type, input.provider, input.name);

	AwsSetCredentialsResult ret;
	if (!credentials.IsExpiredOrEmpty()) {
		// TODO: These should be values?!
		result->secret_map["access_key_id"] = Value(credentials.GetAWSAccessKeyId()).ToString();
		result->secret_map["secret_access_key"] = Value(credentials.GetAWSSecretKey()).ToString();
		result->secret_map["s3_session_token"] = Value(credentials.GetSessionToken()).ToString();
	}

	Aws::ShutdownAPI(options);

	ParseCoreS3Config(input, *result);

	return result;
}

void CreateAwsSecretFunctions::Register(DatabaseInstance &instance) {
	string type = "S3";

	// Register the credential_chain secret provider
	CreateSecretFunction cred_chain_function = {type, "credential_chain", CreateAWSSecretFromCredentialChain};
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

	ExtensionUtil::RegisterFunction(instance, cred_chain_function);
}

} // namespace duckdb
