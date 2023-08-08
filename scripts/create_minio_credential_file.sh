#!/bin/bash

# Set the file path for the credentials file
credentials_file=~/.aws/credentials

# Create the credentials configuration
credentials_config="[default]
aws_access_key_id=minio_duckdb_user
aws_secret_access_key=minio_duckdb_user_password

[minio-testing-2]
aws_access_key_id=minio_duckdb_user_2
aws_secret_access_key=minio_duckdb_user_password_2

[minio-testing-invalid]
aws_access_key_id=minio_duckdb_user_invalid
aws_secret_access_key=thispasswordiscompletelywrong
"

# Write the credentials configuration to the file
echo "$credentials_config" > "$credentials_file"