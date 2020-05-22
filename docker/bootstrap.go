package main

import (
	"context"
	"encoding/json"
	"fmt"
	"log"
	"sandbox/docker/container_manage"
	"sandbox/docker/json_def"
	"sandbox/docker/mq"
)

func main() {
	ctx := context.Background()
	cp := container_manage.ContainerPool{}
	cp.Init(ctx, 4)
	mq.Init()
	log.Printf("Init Finish")
	for {
		valList := mq.RedisCli.BRPop(0, mq.MESSAGEQUEUEKEY)
		v := valList.Val() //[mqName,str]
		if len(v) != 2 {
			log.Printf("task message format error!")
			continue
		}
		jsonStr := v[1]
		var compileAndRunArgs json_def.CompileAndRunArgs
		compileAndRunArgs.SpecialJudgeFile = "" //先初始化，给出默认值
		if err := json.Unmarshal([]byte(jsonStr), &compileAndRunArgs); err != nil {
			log.Println(err)
			continue
		}
		fmt.Printf("runArgs:%+v", compileAndRunArgs)
		entityId := <-cp.AvailableContChans
		log.Printf("entityId:%v\n", entityId)
		go cp.AddTask(ctx, compileAndRunArgs, entityId)
	}
}
