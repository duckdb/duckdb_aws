#define DUCKDB_EXTENSION_MAIN

#include "quack_extension.hpp"
#include "duckdb.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/parser/parsed_data/create_table_function_info.hpp"

#include <aws/core/Aws.h>
#include <aws/core/auth/AWSCredentialsProviderChain.h>
#include <iostream>

namespace duckdb {

//! Set the DuckDB AWS Credentials using the DefaultAWSCredentialsProviderChain
static string TrySetAwsCredentials(DBConfig& config) {
	Aws::SDKOptions options;
	Aws::InitAPI(options);
	Aws::Auth::DefaultAWSCredentialsProviderChain provider;
	auto credentials = provider.GetAWSCredentials();

	string ret;
	if (!credentials.IsExpiredOrEmpty()) {
		config.SetOptionByName("s3_access_key_id", credentials.GetAWSAccessKeyId());
		config.SetOptionByName("s3_access_key_id", credentials.GetAWSSecretKey());
		config.SetOptionByName("s3_session_token", credentials.GetSessionToken());
		ret = credentials.GetAWSAccessKeyId();
	}

	Aws::ShutdownAPI(options);
	return ret;
}

struct SetAWSCredentialsFunctionData : public TableFunctionData {
	bool finished = false;
};

static unique_ptr<FunctionData> LoadAWSCredentialsBind(ClientContext &context, TableFunctionBindInput &input,
                                          vector<LogicalType> &return_types, vector<string> &names) {
	auto result = make_unique<SetAWSCredentialsFunctionData>();
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
		// TODO exception type
		throw Exception("httpfs extension is required for load_aws_credentials");
	}

	//! Return the Key ID of the key we found, or NULL if none was found
	auto key_loaded = TrySetAwsCredentials(DBConfig::GetConfig(context));
	auto ret_val = !key_loaded.empty() ? Value(key_loaded) : Value(nullptr);
	output.SetValue(0,0,ret_val);
	output.SetCardinality(1);

	data.finished = true;
}

static void LoadInternal(DuckDB &db) {
	Connection con(db);

	con.BeginTransaction();
	auto &catalog = Catalog::GetSystemCatalog(*con.context);

	TableFunction load_credentials_func("load_aws_credentials", {}, LoadAWSCredentialsFun, LoadAWSCredentialsBind);
	CreateTableFunctionInfo load_credentials_info(load_credentials_func);
	catalog.CreateTableFunction(*con.context, &load_credentials_info);

	con.Commit();
}

void QuackExtension::Load(DuckDB &db) {
	LoadInternal(db);
}
std::string QuackExtension::Name() {
	return "quack";
}

} // namespace duckdb

extern "C" {

DUCKDB_EXTENSION_API void quack_init(duckdb::DatabaseInstance &db) {
	duckdb::DuckDB db_wrapper(db);
	db_wrapper.LoadExtension<duckdb::QuackExtension>();
}

DUCKDB_EXTENSION_API const char *quack_version() {
	return duckdb::DuckDB::LibraryVersion();
}
}

#ifndef DUCKDB_EXTENSION_MAIN
#error DUCKDB_EXTENSION_MAIN not defined
#endif
