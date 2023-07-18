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


## Examples
### Load AWS Credentials
Input:
```sql
D load aws;
D load httpfs;
D CALL load_aws_credentials()
```
Result:
```sql
D call load_aws_credentials();
┌──────────────────────┐
│      loaded_key      │
│       varchar        │
├──────────────────────┤
│ AKIAIOSFODNN7EXAMPLE │
└──────────────────────┘

```
