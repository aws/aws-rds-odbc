# Limitless Monitoring

## What is Amazon Aurora Limitless Database
Amazon Aurora Limitless Database is a new type of database that can horizontally scale to handle millions of write transactions per second and manage petabytes of data. Users will be able to use the AWS PGSQL ODBC Library with Aurora Limitless Databases and optimize their experience using the Limitless Monitoring Feature. To learn more about Aurora Limitless Database, see the [Amazon Aurora Limitless documentation](https://docs.aws.amazon.com/AmazonRDS/latest/AuroraUserGuide/limitless.html).

## What does the library provide?
Aurora Limitless Database introduces a new endpoint for the databases - the DB shard group (limitless) endpoint that's managed by Route 53. When connecting to Aurora Limitless Database, clients will connect using this endpoint, and be routed to a transaction router via Route 53. Unfortunately, Route 53 is limited in its ability to load balance, and can allow uneven work loads on transaction routers. The library addresses this by performing client-side load balancing with load awareness.

The Limitless Monitoring Feature achieves this by periodically polling for available transaction routers and their load metrics, and then caching them. When a new connection is made, the library allows calls to select a transaction router from the cache using a weighted round-robin strategy. Routers with a higher load are assigned a lower weight, and routers with a lower load are assigned a higher weight.

## Use with Other Features
The Limitless Connection Feature is compatible with authentication type features such as the IAM and AWS Secrets Manager.

> [!WARNING]\
> The Failover feature is also compatible with the Limitless Connection Feature.  
However, we don't recommend using them with the Limitless Connection Feature because they're not designed to be used with Aurora Limitless Database. 
They don't provide any extra value, and add unnecessary computation and memory overhead.

## Sample Code
[Limitless Example](./limitless_example.cpp)
