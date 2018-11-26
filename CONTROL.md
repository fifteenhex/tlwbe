# control interface

tlwbe/control/app/add
tlwbe/control/app/update
tlwbe/control/app/del
tlwbe/control/app/list
tlwbe/control/app/get

tlwbe/control/dev/add
tlwbe/control/dev/update
tlwbe/control/dev/del

## list devs

```tlwbe/control/dev/list```

```{ "token": "<uuid or other unique string>" }```

```{"token":"<your token>","result":...,"code":0}```

## get dev

```tlwbe/control/dev/get```

```{ "token": "<uuid or other unique string>", "eui": "<device eui>" }```

tlwbe/control/result