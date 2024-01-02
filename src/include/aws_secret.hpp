#pragma once

#include "aws_extension.hpp"
#include "duckdb.hpp"
#include "duckdb/main/secret/secret.hpp"

namespace duckdb {

struct CreateAwsSecretFunctions {
public:
	//! Register all CreateSecretFunctions
	static void Register(DatabaseInstance &instance);
};

} // namespace duckdb
