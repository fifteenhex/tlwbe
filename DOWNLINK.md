# downlink interface

## queuing downlinks

### publish topic

```
tlwbe/downlink/schedule/<appeui>/<deveui>/<port>/<token>
```

### publish payload

```
{"confirm": false, "payload": <base64 encoded payload>}

```

## querying queued downlinks

### publish topic

```
tlwbe/downlink/query/<token>
```

## Results

```
tlwbe/downlink/result/<token>
```

## downlink transmission announcement

### publish topic

```
tlwbe/downlink/sent/<token>
```