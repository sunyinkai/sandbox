package container_manage

import (
	"context"
	"github.com/docker/docker/api/types"
	"github.com/docker/docker/api/types/container"
	"github.com/docker/docker/api/types/filters"
	"log"
	"sandbox/docker/json_def"
	"sandbox/docker/utils"
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
			log.Printf("create container entity error,error is %s", err.Error())
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
	// container.config
	containerConfig := &container.Config{
		Image:        image,
		Cmd:          []string{},
		AttachStdin:  true,
		AttachStdout: true,
		AttachStderr: true,
		OpenStdin:    true,
		User:         "0", //uid
		//NetworkDisabled: true, //ban掉网络
	}

	//host.config
	availableCap := []string{"CAP_SETUID", "CAP_SETGID", "CAP_CHOWN"} //限制root能力
	hostConfig := &container.HostConfig{
		Capabilities: availableCap,
		Resources: container.Resources{
			Memory:     256 * 1024 * 1024, //256M
			MemorySwap: 256 * 1024 * 1024, //0M
		}, // 限制内存使用
	}

	cont, err := cli.ContainerCreate(
		ctx,
		containerConfig,
		hostConfig,
		nil,"")
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

//检查容器状态
func (cp *ContainerPool) CheckContainerState(ctx context.Context, contId string) (string, error) {
	opts := types.ContainerListOptions{All: true}
	opts.Filters = filters.NewArgs()
	opts.Filters.Add("id", contId)
	contList, err := cli.ContainerList(ctx, opts)
	if err != nil {
		log.Printf("cli.ContainerList error,error is %s", err.Error())
		return "", err
	} else {
		if len(contList) > 0 {
			myCont := contList[0]
			log.Printf("myCont:%+v", myCont)
			return myCont.State, nil
		} else {
			return "notFound", nil
		}
	}
}

//更新容器
func (cp *ContainerPool) UpdateIndex(ctx context.Context, index int) {
	state, err := cp.CheckContainerState(ctx, cp.containerList[index].contId)
	if err != nil {
		panic(err)
	}
	if state != "running" {
		log.Printf("contId:%s,status:%s", cp.containerList[index].contId, state)
		const maxRetryTime = 3
		var retry = 0
		for retry = 0; retry < maxRetryTime; retry += 1 {
			contId, err := cp.CreateContainerEntity(ctx, "ubuntu:sandbox")
			if err != nil {
				log.Printf("create cont error")
			} else {
				cp.containerList[index].contId = contId
				cp.containerList[index].StartContainerEntity(ctx)
				log.Printf("poolList index %d now %s", index, contId)
				break
			}
		}
		if retry == maxRetryTime {
			panic("update index reach max retry time")
		}
	}
}

func (cp *ContainerPool) AddTask(ctx context.Context, compileAndRunArgs json_def.CompileAndRunArgs, index int) {
	defer func() {
		cp.containerList[index].Reset()
		cp.AvailableContChans <- index
	}()
	cp.UpdateIndex(ctx, index)
	cp.containerList[index].randStr = utils.GenRandomStr(20)
	cp.containerList[index].BuildEnvAndRun(ctx, &compileAndRunArgs)
}
