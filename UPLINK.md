# Uplink interface

This interface allows for you to monitor live uplinks as they
come in from devs and also query uplinks tlwbe has stored.

## Live uplinks from devices

### publish topic

```tlwbe/uplink/<appeui>/<deveui>```

## Querying stored uplinks

### publish topic

```tlwbe/uplinks/query/<token; uuid or other unique string>```

### publish payload

```
{
	"deveui": "<dev eui to get uplinks for>"
}
```

### result topic

```tlwbe/uplinks/result/<token from query>```

### result payload

```
{
	"uplinks":[...]
}
```
