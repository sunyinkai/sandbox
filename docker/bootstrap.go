package main

import (
	"context"
	"encoding/json"
	"log"
	"sandbox/docker/container_manage"
	"sandbox/docker/json_def"
	"sandbox/docker/mq"
)

func main() {
	ctx := context.Background()
	cp := container_manage.ContainerPool{}
	cp.Init(ctx, 2)
	mq.Init()
	log.Printf("Init Finish")
	for {
		valList := mq.RedisCli.BRPop(0, mq.MESSAGEQUEUEKEY)
		v := valList.Val() //[mqName,str]
		jsonStr := v[1]
		var compileAndRunArgs json_def.CompileAndRunArgs
		if err := json.Unmarshal([]byte(jsonStr), &compileAndRunArgs); err != nil {
			log.Println(err)
			continue
		}

		entityId := <-cp.AvailableContChans
		log.Printf("entityId:%v\n", entityId)
		go cp.AddTask(ctx, compileAndRunArgs, entityId)
	}
}
