package main

import (
	"context"
	"fmt"
	"github.com/docker/docker/api/types"
	"github.com/docker/docker/api/types/container"
	"github.com/docker/docker/client"
	"github.com/docker/docker/pkg/stdcopy"
	"github.com/docker/go-connections/nat"
	"io"
	"os"
	"strings"
)

var cli *client.Client

type ContainerInstance struct {
	contId string
}

func init() {
	cli, _ = client.NewClientWithOpts(client.WithVersion("1.40"))
}

func (ci *ContainerInstance) CreateNewContainer(ctx context.Context, image string) (string, error) {
	hostBinding := nat.PortBinding{
		HostIP:   "0.0.0.0",
		HostPort: "8000",
	}
	containerPort, err := nat.NewPort("tcp", "80")
	if err != nil {
		panic("unable to get to port")
	}
	var portBind nat.PortMap = nat.PortMap{containerPort: []nat.PortBinding{hostBinding}}
	// container.config
	containerConfig := &container.Config{
		Image: image,
		Cmd:   []string{"echo", "hello,world", ">", "a.txt"},
	}
	//host.config
	hostConfig := &container.HostConfig{
		PortBindings: portBind,
		Resources:    container.Resources{Memory: 3e+7}, // 限制内存使用
	}
	cont, err := cli.ContainerCreate(
		ctx,
		containerConfig,
		hostConfig,
		nil, "")
	if err != nil {
		panic(err)
	}
	return cont.ID, nil
}

//执行命令
func (ci *ContainerInstance) RunCmdInContainer(ctx context.Context, cmdList []string) error {
	//启动容器
	if err := cli.ContainerStart(ctx, ci.contId, types.ContainerStartOptions{}); err != nil {
		panic(err)
	}

	//创建命令
	for _, cmd := range cmdList {
		exeOpt := types.ExecConfig{
			AttachStdout: true,
			AttachStderr: true,
			Cmd:          strings.Fields(strings.TrimSpace(cmd)),
		}
		exeId, err := cli.ContainerExecCreate(ctx, ci.contId, exeOpt)
		if err != nil {
			fmt.Println(err.Error())
			continue
		}
		fmt.Println("exe id is ", exeId)
		resp, er := cli.ContainerExecAttach(ctx, exeId.ID, types.ExecStartCheck{})
		if er != nil {
			fmt.Println(er.Error())
		}
		err = cli.ContainerExecStart(ctx, exeId.ID, types.ExecStartCheck{})
		if err != nil {
			fmt.Println(err.Error())
		}

		stdcopy.StdCopy(os.Stdout, os.Stdin, resp.Reader)
		resp.Close()
	}
	return nil
}

//删除容器
func (ci *ContainerInstance) RmContainer(ctx context.Context) {
	if err := cli.ContainerRemove(ctx, ci.contId, types.ContainerRemoveOptions{}); err != nil {
		panic(err)
	}
}

//关掉容器
func (ci *ContainerInstance) StopContainer(ctx context.Context) {
	if err := cli.ContainerStop(ctx, ci.contId, nil); err != nil {
		panic(err)
	}
}

//往容器里复制文件
func (ci *ContainerInstance) CopyFileToContainer(ctx context.Context, srcFile string, dstPath string) {
	var content io.Reader
	err := cli.CopyToContainer(ctx, ci.contId, dstPath, content, types.CopyToContainerOptions{AllowOverwriteDirWithFile: true})
	if err != nil {
		panic(err.Error())
	}
}

//从容器里拷贝出文件
func (ci *ContainerInstance) CopyFileFromContainer(ctx context.Context, srcFile string, dstPath string) {
	readCloser, containerPathStatus, err := cli.CopyFromContainer(ctx, ci.contId, srcFile)
	if err != nil {
		panic(err.Error())
	}
	fmt.Printf("%+v", containerPathStatus)
	var p []byte
	readCloser.Read(p)
	readCloser.Close()
}

func main() {
	ctx := context.Background()
	//contId, _ := CreateNewContainer(ctx, "mydocker:python")
	contId := "da3db1907d1e"
	ci := ContainerInstance{contId: contId}
	var cmdList = []string{"ls -ltr", "touch /tmp/ttl.txt"}
	_ = ci.RunCmdInContainer(ctx, cmdList)
	ci.StopContainer(ctx)
	fmt.Println("contId is ", contId)
}
