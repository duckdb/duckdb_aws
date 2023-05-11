# DuckDB AWS Extension
This is a DuckDB extension that provides features that depend on the AWS SDK.

## Supported architectures
Currently, only Linux x86_64 and MacOS are supported.

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
