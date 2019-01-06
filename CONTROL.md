# control interface

This interface is used to do CRUD operations on apps and devs
so you can add apps, and then devs to apps, set/update keys
etc.

## Basics

Requests are made by sending a publish to one of the topics
detailed below. The last element in the topic should be a
token like a uuid that your client can use to extract the
result from the flow of results tlwbe publishes to the [results](#Results)
topic.

## Managing Applications

### Adding an app

To add an app you only need to provide a unique app name to the
`app/add` endpoint. If an existing app is being re-added
so some reason you can provide the existing EUI for it.

#### publish topic

```
tlwbe/control/app/add/<token; uuid or other unique string>
```

#### publish payload

```
{ "name": <app name>, "eui": <optional eui>}
```

#### result payload

See *get app* for the contents of "app".

```
{ "code": 0, "app": { .. } }
```

### Updating an app

#### publish topic

```
tlwbe/control/app/update/<token; uuid or other unique string>
```

### Deleting an app

#### publish topic

```
tlwbe/control/app/del/<token; uuid or other unique string>
```

### Listing apps

```
tlwbe/control/app/list/<token; uuid or other unique string>
```

### Get a specific app

```
tlwbe/control/app/get/<token; uuid or other unique string>
```

```
{ "eui": "<app eui>" }
```
## Managing Devices

### Adding a device

#### publish topic

```
tlwbe/control/dev/add
```

### updating a device

#### publish topic

```
tlwbe/control/dev/update
```

### Deleting a device

#### publish topic

```
tlwbe/control/dev/del
```

### Listing devices

#### publish topic

```
tlwbe/control/dev/list/<token; uuid or other unique string>
```

```
{ }
```

```
{"eui_list": [ ... ] ,"code":0}
```

### Getting a specific device

#### publish topic

```
tlwbe/control/dev/get/<token; uuid or other unique string>
```

```
{"eui": "<device eui>" }
```

## Request results

```
tlwbe/control/result/<token; uuid or other unique string>
```