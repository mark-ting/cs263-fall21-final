# SecureSub: Hardware Secured Pub/Sub Subscription Lists using SGX Enclaves
Authors: Tristan Yang, Mark Ting, Dylan Li and Bryan Lee
Harvard CS 263, Fall 2021

This code creates a basic pub/sub system involving three components: publishers, subscribers, and a central broker server which coordinates messages.

# Code Structure
The code is segmented into the three pub/sub system components, the publishers (`publisher.cpp`), the subscriber (`subscriber.cpp`) and the broker (`broker.cpp`).

To run the code:
1. Run `make all` to build all components

In separate terminal windows for each step,
2. Run `./broker` to start the broker server
3. Run any number of instances of `./publisher` to run multiple publishers
4. Run any number of instances of `./subscriber` to run multiple subscribers

## Publishers
To publish messages. In any terminal instance running `./publisher`, type any string shorter than 512 characters. This will be sent to the `./broker` and will be routed to any active subscribers whose filters are met.

To close the publisher-broker connection, type `Ctrl+C`. 

## Subscribers
By default, all subscribers receive all messages since they have no content filters. To limit the scope of messages subscribed to, in any terminal running `./subscriber`, type a regular expression string which will add a filter. Now only published content which meets the filters will be routed.

Common regex expressions to test include: 
- ADD REGEX EXPRESSIONS

Note that regex expression format is not validated in this version, so users should only input valid regex expressions. 
