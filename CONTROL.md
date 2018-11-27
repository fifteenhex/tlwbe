# control interface

## add app

### publish topic

```tlwbe/control/app/add/<token; uuid or other unique string>```

### publish payload

```{}```


## update app

```tlwbe/control/app/update/<token; uuid or other unique string>```

## delete app

```tlwbe/control/app/del/<token; uuid or other unique string>```

## list apps

```tlwbe/control/app/list/<token; uuid or other unique string>```

## get app

```tlwbe/control/app/get/<token; uuid or other unique string>```

```{ "eui": "<app eui>" }```


## add dev

```tlwbe/control/dev/add```

## update dev

```tlwbe/control/dev/update```

## delete dev

```tlwbe/control/dev/del```

## list devs

```tlwbe/control/dev/list/<token; uuid or other unique string>```

```{ }```

```{"result":...,"code":0}```

## get dev

```tlwbe/control/dev/get/<token; uuid or other unique string>```

```{"eui": "<device eui>" }```

## result topic

```tlwbe/control/result/<token; uuid or other unique string>```