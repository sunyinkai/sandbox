package main

import (
	"context"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"sandbox/docker/container_manage"
	"sandbox/docker/json_def"
	"time"
)

func main() {

	//var contId = "294672129a10"
	var contId = ""
	ctx := context.Background()
	fmt.Println(time.Now().Format("2006-01-02 15:04:05"))
	var ci container_manage.ContainerInstance
	ci.ContId = ""
	if len(contId) == 0 {
		contId, _ = ci.CreateNewContainer(ctx, "ubuntu:sandbox")
		ci.ContId = contId
		fmt.Printf("contId is %s", contId)
	} else {
		ci.ContId = contId
	}

	var err error
	var jsonByte []byte
	var compileAndRunArgs json_def.CompileAndRunArgs
	if jsonByte, err = ioutil.ReadFile("config.json"); err != nil {
		panic(err)
	}
	if err = json.Unmarshal(jsonByte, &compileAndRunArgs); err != nil {
		panic(err)
	}
	fmt.Printf("%+v", compileAndRunArgs)
	ci.BuildDockerRunEnv(ctx, &compileAndRunArgs)
}
