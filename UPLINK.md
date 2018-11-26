# Uplink interface

Live uplinks from devices
tlwbe/uplink/<appeui>/<deveui>

## Querying stored uplinks

```tlwbe/uplinks/query/<token; uuid or other unique string>```

```
{
	"deveui": "<dev eui to get uplinks for"
}
```

```tlwbe/uplinks/result/<token from query>```

```
{"uplinks":[...]}
```

tlwbe/uplinks/result/<token from query>
{
	"token": <token from query>
}