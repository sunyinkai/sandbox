package main

import (
	"context"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"sandbox/docker/container_manage"
	"sandbox/docker/json_def"
	"sandbox/docker/utils"
	"time"
)

func main() {

	ctx := context.Background()
	fmt.Println(time.Now().Format("2006-01-02 15:04:05"))
	var ci container_manage.ContainerInstance
	ci.ContId = "463879cfe511"
	if len(ci.ContId) == 0 {
		contId, _ := ci.CreateNewContainer(ctx, "ubuntu:sandbox")
		ci.ContId = contId
		ci.RandStr = utils.GenRandomStr(20)
		fmt.Printf("contId is %s,randStr is %s", ci.ContId, ci.RandStr)
	} else {
		ci.RandStr = utils.GenRandomStr(20)
		fmt.Printf("contId is %s,randStr is %s", ci.ContId, ci.RandStr)
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
	fmt.Printf("%+v\n", compileAndRunArgs)
	ci.BuildDockerRunEnv(ctx, &compileAndRunArgs)
}
