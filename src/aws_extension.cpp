#define DUCKDB_EXTENSION_MAIN

#include "aws_extension.hpp"
#include "duckdb.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/main/extension_util.hpp"
#include <duckdb/parser/parsed_data/create_scalar_function_info.hpp>
#include <aws/core/Aws.h>
#include <aws/core/auth/AWSCredentialsProviderChain.h>
#include <iostream>

namespace duckdb {

//! Set the DuckDB AWS Credentials using the DefaultAWSCredentialsProviderChain
static string TrySetAwsCredentials(DBConfig& config, const string& profile) {
	Aws::SDKOptions options;
	Aws::InitAPI(options);
	Aws::Auth::AWSCredentials credentials;

	if (!profile.empty()) {
		// The user has specified a specific profile they want to use instead of the current profile specified by the
		// system
		Aws::Auth::ProfileConfigFileAWSCredentialsProvider provider(profile.c_str());
		credentials = provider.GetAWSCredentials();
	} else {
		Aws::Auth::DefaultAWSCredentialsProviderChain provider;
		credentials = provider.GetAWSCredentials();
	}

	string ret;
	if (!credentials.IsExpiredOrEmpty()) {
		config.SetOption("s3_access_key_id", Value(credentials.GetAWSAccessKeyId()));
		config.SetOption("s3_secret_access_key", Value(credentials.GetAWSSecretKey()));
		config.SetOption("s3_session_token", Value(credentials.GetSessionToken()));
		ret = credentials.GetAWSAccessKeyId();
	}

	Aws::ShutdownAPI(options);
	return ret;
}

struct SetAWSCredentialsFunctionData : public TableFunctionData {
	string profile_name;
	bool finished = false;
};

static unique_ptr<FunctionData> LoadAWSCredentialsBind(ClientContext &context, TableFunctionBindInput &input,
                                          vector<LogicalType> &return_types, vector<string> &names) {
	auto result = make_uniq<SetAWSCredentialsFunctionData>();

	if (input.inputs.size() >= 1) {
		result->profile_name = input.inputs[0].ToString();
	}

	return_types.emplace_back(LogicalType::VARCHAR);
	names.emplace_back("loaded_key");
	return std::move(result);
}

static void LoadAWSCredentialsFun(ClientContext &context, TableFunctionInput &data_p, DataChunk &output) {
	auto &data = (SetAWSCredentialsFunctionData &)*data_p.bind_data;
	if (data.finished) {
		return;
	}

	if (!context.db->ExtensionIsLoaded("httpfs")) {
		throw MissingExtensionException("httpfs extension is required for load_aws_credentials");
	}

	//! Return the Key ID of the key we found, or NULL if none was found
	auto key_loaded = TrySetAwsCredentials(DBConfig::GetConfig(context), data.profile_name);
	auto ret_val = !key_loaded.empty() ? Value(key_loaded) : Value(nullptr);
	output.SetValue(0,0,ret_val);
	output.SetCardinality(1);

	data.finished = true;
}

static void LoadInternal(DuckDB &db) {
	TableFunctionSet function_set("load_aws_credentials");
	function_set.AddFunction(TableFunction("load_aws_credentials", {}, LoadAWSCredentialsFun, LoadAWSCredentialsBind));
	function_set.AddFunction(TableFunction("load_aws_credentials", {LogicalTypeId::VARCHAR}, LoadAWSCredentialsFun, LoadAWSCredentialsBind));
	ExtensionUtil::RegisterFunction(*db.instance, function_set);
}

void AwsExtension::Load(DuckDB &db) {
	LoadInternal(db);
}
std::string AwsExtension::Name() {
	return "aws";
}

} // namespace duckdb

extern "C" {

DUCKDB_EXTENSION_API void aws_init(duckdb::DatabaseInstance &db) {
	duckdb::DuckDB db_wrapper(db);
	db_wrapper.LoadExtension<duckdb::AwsExtension>();
}

DUCKDB_EXTENSION_API const char *aws_version() {
	return duckdb::DuckDB::LibraryVersion();
}
}

#ifndef DUCKDB_EXTENSION_MAIN
#error DUCKDB_EXTENSION_MAIN not defined
#endif
