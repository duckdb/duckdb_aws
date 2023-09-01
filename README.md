## Experimental warning
This extension is currently in an experimental state. Feel free to try it out, but be aware some things
may not work as expected.

# DuckDB AWS Extension
This is a DuckDB extension that provides features that depend on the AWS SDK.

## Binary distribution
Binaries are available in the main extension repository for DuckDB only for nightly builds at the moment, but will be 
available next release of DuckDB (v0.9.0)

## Supported architectures
The extension is tested & distributed for Linux (x64), MacOS (x64, arm64) and Windows (x64)

## Features
| function | type | description | 
| --- | --- | --- |
| `load_aws_credentials` | Pragma call function | Automatically loads the AWS credentials through the Default AWS Credentials Provider Chain |


## Usage
### Load AWS Credentials
Firstly ensure the `aws` and `httpfs` extensions are loaded and installed:
```sql
D install aws; load aws; install httpfs; load httpfs;
```
Then to load the aws credentials run:
```sql
D call load_aws_credentials();
┌─────────────────────────┬──────────────────────────┬──────────────────────┬───────────────┐
│ loaded_access_key_id    │ loaded_secret_access_key │ loaded_session_token │ loaded_region │
│       varchar           │         varchar          │       varchar        │    varchar    │
├─────────────────────────┼──────────────────────────┼──────────────────────┼───────────────┤
│ AKIAIOSFODNN7EXAMPLE    │ <redacted>               │                      │ eu-west-1     │
└─────────────────────────┴──────────────────────────┴──────────────────────┴───────────────┘
```

The function takes a string parameter to specify a specific profile:
```sql
D call load_aws_credentials('minio-testing-2');
┌──────────────────────┬──────────────────────────┬──────────────────────┬───────────────┐
│ loaded_access_key_id │ loaded_secret_access_key │ loaded_session_token │ loaded_region │
│       varchar        │         varchar          │       varchar        │    varchar    │
├──────────────────────┼──────────────────────────┼──────────────────────┼───────────────┤
│ minio_duckdb_user_2  │ <redacted>               │                      │ eu-west-2     │
└──────────────────────┴──────────────────────────┴──────────────────────┴───────────────┘
```

There are several parameters to tweak the behaviour of the call:
```sql
D call load_aws_credentials('minio-testing-2', set_region=false, redact_secret=false);
┌──────────────────────┬──────────────────────────────┬──────────────────────┬───────────────┐
│ loaded_access_key_id │   loaded_secret_access_key   │ loaded_session_token │ loaded_region │
│       varchar        │           varchar            │       varchar        │    varchar    │
├──────────────────────┼──────────────────────────────┼──────────────────────┼───────────────┤
│ minio_duckdb_user_2  │ minio_duckdb_user_password_2 │                      │               │
└──────────────────────┴──────────────────────────────┴──────────────────────┴───────────────┘

```