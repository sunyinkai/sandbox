package container_manage

import (
	"context"
	"fmt"
	"github.com/docker/docker/api/types"
	"github.com/docker/docker/api/types/container"
	"github.com/docker/go-connections/nat"
	"log"
	"sandbox/docker/json_def"
	"sandbox/docker/utils"
	"time"
)

type ContainerPool struct {
	containerList      []*ContainerEntity
	poolSize           int
	AvailableContChans chan int //可用container
}

func (cp *ContainerPool) Init(ctx context.Context, size int) {
	cp.AvailableContChans = make(chan int, size)
	cp.containerList = make([]*ContainerEntity, 0)
	cp.poolSize = size
	for i := 0; i < size; i += 1 {
		contId, err := cp.CreateContainerEntity(ctx, "ubuntu:sandbox")
		if err != nil {
			panic(err)
		}
		ce := &ContainerEntity{
			contId:     contId,
			isOccupied: false,
		}
		cp.containerList = append(cp.containerList, ce)
		ce.StartContainerEntity(ctx) //启动容器　
		cp.AvailableContChans <- i
	}
	log.Printf("cp info:%+v", cp)
}

//创建容器
func (cp *ContainerPool) CreateContainerEntity(ctx context.Context, image string) (string, error) {
	hostBinding := nat.PortBinding{
		HostIP:   "0.0.0.0",
		HostPort: "8000",
	}
	containerPort, err := nat.NewPort("tcp", "80")
	if err != nil {
		panic("unable to get to port")
	}
	var portBind = nat.PortMap{containerPort: []nat.PortBinding{hostBinding}}

	// container.config
	containerConfig := &container.Config{
		Image:           image,
		Cmd:             []string{},
		AttachStdin:     true,
		AttachStdout:    true,
		AttachStderr:    true,
		OpenStdin:       true,
		User:            "0",  //uid
		//NetworkDisabled: true, //ban掉网络
	}

	//host.config
	//availableCap := []string{"CAP_SETUID", "CAP_SETGID", "CAP_CHOWN", "CAP_KILL"} //限制root能力

	hostConfig := &container.HostConfig{
		PortBindings: portBind,
		//Capabilities: availableCap,
		Resources: container.Resources{
			Memory:     1e8,
			MemorySwap: 1e8,
		}, // 限制内存使用
	}

	containerName := fmt.Sprintf("%d", time.Now().Unix())
	cont, err := cli.ContainerCreate(
		ctx,
		containerConfig,
		hostConfig,
		nil, containerName)
	if err != nil {
		panic(err)
	}
	return cont.ID, nil
}

//删除容器
func (cp *ContainerPool) RmContainerEntity(ctx context.Context, contId string) {
	if err := cli.ContainerRemove(ctx, contId, types.ContainerRemoveOptions{}); err != nil {
		panic(err)
	}
}

func (cp *ContainerPool) AddTask(ctx context.Context, compileAndRunArgs json_def.CompileAndRunArgs, index int) {
	defer func() {
		cp.containerList[index].Reset()
		cp.AvailableContChans <- index
	}()
	cp.containerList[index].randStr = utils.GenRandomStr(20)
	cp.containerList[index].BuildEnvAndRun(ctx, &compileAndRunArgs)
}
