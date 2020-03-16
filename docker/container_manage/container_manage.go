package container_manage

import (
	"bytes"
	"context"
	"fmt"
	"github.com/docker/docker/api/types"
	"github.com/docker/docker/api/types/container"
	"github.com/docker/docker/client"
	"github.com/docker/docker/pkg/stdcopy"
	"github.com/docker/go-connections/nat"
	"io"
	"io/ioutil"
	"os"
	"sandbox/docker/json_def"
	"sandbox/docker/utils"
	"strings"
	"time"
)

var cli *client.Client

const (
	DOCKERWORKPATH = "/home/sandbox"
	DOCKEREXENAME  = "runner_FSM_docker.out"
)

type ContainerInstance struct {
	ContId  string
	RandStr string // 随机字符串
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
		NetworkDisabled: true, //ban掉网络
	}

	//host.config
	availableCap := []string{"CAP_SETUID", "CAP_SETGID", "CAP_CHOWN", "CAP_KILL"} //限制root能力

	hostConfig := &container.HostConfig{
		PortBindings: portBind,
		Capabilities: availableCap,
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
func (ci *ContainerInstance) RmContainer(ctx context.Context) {
	if err := cli.ContainerRemove(ctx, ci.ContId, types.ContainerRemoveOptions{}); err != nil {
		panic(err)
	}
}

//启动容器
func (ci *ContainerInstance) StartContainer(ctx context.Context) {
	//启动容器
	if err := cli.ContainerStart(ctx, ci.ContId, types.ContainerStartOptions{}); err != nil {
		panic(err)
	}
}

//停止容器
func (ci *ContainerInstance) StopContainer(ctx context.Context) {
	timeout := time.Second * 0 //经过timeout发送kill信号
	if err := cli.ContainerStop(ctx, ci.ContId, &timeout); err != nil {
		panic(err)
	}
}

//执行命令
func (ci *ContainerInstance) RunCmdInContainer(ctx context.Context, cmd string) error {
	//创建命令
	var exec = []string{"bash", "-c"}
	exec = append(exec, cmd)

	exeOpt := types.ExecConfig{
		AttachStdout: true,
		AttachStderr: true,
		AttachStdin:  true,
		Tty:          true,
		Cmd:          exec,
	}
	exeId, err := cli.ContainerExecCreate(ctx, ci.ContId, exeOpt)
	if err != nil {
		fmt.Println(err.Error())
		panic(err)
	}
	fmt.Println("exe id is ", exeId)
	resp, er := cli.ContainerExecAttach(ctx, exeId.ID, types.ExecStartCheck{})
	if er != nil {
		fmt.Println(er.Error())
		panic(err)
	}
	err = cli.ContainerExecStart(ctx, exeId.ID, types.ExecStartCheck{})
	if err != nil {
		fmt.Printf("there is an error")
		fmt.Println(err.Error())
	}
	_, _ = stdcopy.StdCopy(os.Stdout, os.Stdout, resp.Reader)

	resp.Close()
	return nil
}

//往容器里复制文件
func (ci *ContainerInstance) CopyFileToContainer(ctx context.Context, srcFile string, dstPath string) {
	var content io.Reader
	targetFile := fmt.Sprintf("%s.tar", srcFile)
	_ = utils.TarFile(srcFile, targetFile)
	content, _ = os.Open(targetFile)
	err := cli.CopyToContainer(ctx, ci.ContId, dstPath, content, types.CopyToContainerOptions{AllowOverwriteDirWithFile: true})
	if err != nil {
		panic(err.Error())
	}
}

//从容器里拷贝出文件
func (ci *ContainerInstance) CopyFileFromContainer(ctx context.Context, srcFile string, dstPath string) {
	readCloser, containerPathStatus, err := cli.CopyFromContainer(ctx, ci.ContId, srcFile)
	defer readCloser.Close()
	if err != nil {
		panic(err.Error())
	}

	fmt.Printf("%+v", containerPathStatus)
	buf := new(bytes.Buffer)
	_, _ = buf.ReadFrom(readCloser)
	tmpTarFile := fmt.Sprintf("/tmp/result_%s.tar", ci.RandStr)
	err = ioutil.WriteFile(tmpTarFile, buf.Bytes(), 0666)
	defer os.Remove(tmpTarFile)
	_ = utils.Untar(tmpTarFile, dstPath)
	if err != nil {
		panic(err)
	}
}

var ContId = ""

//将相关文件传入docker内,构建运行环境
func (ci *ContainerInstance) BuildDockerRunEnv(ctx context.Context, args *json_def.CompileAndRunArgs) {
	ci.StartContainer(ctx)
	//创建临时目录
	absolutePath := fmt.Sprintf("%s/%s", DOCKERWORKPATH, ci.RandStr)
	mkdirStr := fmt.Sprintf("mkdir -p %s", absolutePath)
	_ = ci.RunCmdInContainer(ctx, mkdirStr)

	//将文件拷贝到docker内去
	ci.CopyFileToContainer(ctx, args.SourceFile, absolutePath)
	ci.CopyFileToContainer(ctx, args.SysInputFile, absolutePath)
	ci.CopyFileToContainer(ctx, args.SysOutputFile, absolutePath)

	//执行相关命令
	chmodStr := fmt.Sprintf("chown root:root -R %s", absolutePath)

	//将dockerExe以及config目录放到absolute目录下
	copyExeAndConfigStr := fmt.Sprintf("cp %s %s && cp -r %s %s", DOCKEREXENAME, absolutePath, "config", absolutePath)
	_ = ci.RunCmdInContainer(ctx, copyExeAndConfigStr)

	executableName := fmt.Sprintf("%s/%s", absolutePath, DOCKEREXENAME) //runner_FSM 所在位置
	userExeName := fmt.Sprintf("%s/%s_%s", absolutePath, ci.RandStr, args.Language)
	userOutputFile := fmt.Sprintf("%s/%s.output", absolutePath, ci.RandStr)
	getFileNameInHost := func(input string) string {
		stringSlice := strings.Split(input, "/")
		return stringSlice[len(stringSlice)-1]
	}
	sysInputFileInDocker := fmt.Sprintf("%s/%s", absolutePath, getFileNameInHost(args.SysInputFile))
	sysOutputFileInDocker := fmt.Sprintf("%s/%s", absolutePath, getFileNameInHost(args.SysOutputFile))
	usrSourceFileInDocker := fmt.Sprintf("%s/%s", absolutePath, getFileNameInHost(args.SourceFile))
	exeRunnerStr := fmt.Sprintf("%s %s %s %d %d %d %s %s %s %s",
		executableName, args.Language, usrSourceFileInDocker,
		args.Time, args.Memory, args.Disk,
		userExeName, sysInputFileInDocker, sysOutputFileInDocker, userOutputFile)
	fmt.Printf("\nrunnerStr:%s\n", exeRunnerStr)
	unionChmodAndExeRunner := fmt.Sprintf("%s && %s", chmodStr, exeRunnerStr)
	_ = ci.RunCmdInContainer(ctx, unionChmodAndExeRunner)

	//获取docker内部的json文件
	jsonPathInDocker := fmt.Sprintf("%s/%s", DOCKERWORKPATH, "result.json")
	ci.CopyFileFromContainer(ctx, jsonPathInDocker, "./result.json")
}
