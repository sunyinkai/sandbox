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
	"sandbox/docker/utils"
	"time"
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
		Image:           image,
		Cmd:             []string{},
		NetworkDisabled: true, //ban掉网络
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

//删除容器
func (ci *ContainerInstance) RmContainer(ctx context.Context) {
	if err := cli.ContainerRemove(ctx, ci.contId, types.ContainerRemoveOptions{}); err != nil {
		panic(err)
	}
}

//启动容器
func (ci *ContainerInstance) StartContainer(ctx context.Context) {
	//启动容器
	if err := cli.ContainerStart(ctx, ci.contId, types.ContainerStartOptions{}); err != nil {
		panic(err)
	}
}

//停止容器
func (ci *ContainerInstance) StopContainer(ctx context.Context) {
	if err := cli.ContainerStop(ctx, ci.contId, nil); err != nil {
		panic(err)
	}
}

//执行命令
func (ci *ContainerInstance) RunCmdInContainer(ctx context.Context, cmdList []string) error {
	//创建命令
	for _, cmd := range cmdList {
		var exec = []string{"bash", "-c"}
		exec = append(exec, cmd)

		exeOpt := types.ExecConfig{
			AttachStdout: true,
			AttachStderr: true,
			AttachStdin:  true,
			Tty:          true,
			Cmd:          exec,
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
			continue
		}
		err = cli.ContainerExecStart(ctx, exeId.ID, types.ExecStartCheck{})
		if err != nil {
			fmt.Println(err.Error())
		}

		_, _ = stdcopy.StdCopy(os.Stdout, os.Stdin, resp.Reader)
		resp.Close()
	}
	return nil
}

//往容器里复制文件
func (ci *ContainerInstance) CopyFileToContainer(ctx context.Context, srcFile string, dstPath string) {
	var content io.Reader
	targetFile := fmt.Sprintf("%s.tar", srcFile)
	_ = utils.TarFile(srcFile, targetFile)
	content, _ = os.Open(targetFile)
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
	contId := "da3db1907d1e"
	srcFile := "/home/naoh/Github/sandbox/output/a.py"
	targetPath := "/"
	var cmdList = []string{
		"whoami",
		"ls -ltr",
		"exit",
	}

	ctx := context.Background()
	// 初始化containerInstance
	ci := ContainerInstance{contId: contId}

	ci.StartContainer(ctx)
	fmt.Println(time.Now().Clock())
	ci.CopyFileToContainer(ctx, srcFile, targetPath)
	_ = ci.RunCmdInContainer(ctx, cmdList)
	ci.StopContainer(ctx)
	fmt.Println(time.Now().Clock())
	fmt.Println("contId is ", contId)
}
