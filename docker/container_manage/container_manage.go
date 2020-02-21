package container_manage

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
	ContId string
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
	availableCap := []string{"CAP_SETUID", "CAP_SETGID", "CAP_CHOWN","CAP_KILL"} //限制root能力

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
		exeId, err := cli.ContainerExecCreate(ctx, ci.ContId, exeOpt)
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
	err := cli.CopyToContainer(ctx, ci.ContId, dstPath, content, types.CopyToContainerOptions{AllowOverwriteDirWithFile: true})
	if err != nil {
		panic(err.Error())
	}
}

//从容器里拷贝出文件
func (ci *ContainerInstance) CopyFileFromContainer(ctx context.Context, srcFile string, dstPath string) {
	readCloser, containerPathStatus, err := cli.CopyFromContainer(ctx, ci.ContId, srcFile)
	if err != nil {
		panic(err.Error())
	}
	fmt.Printf("%+v", containerPathStatus)
	var p []byte
	_, _ = readCloser.Read(p)
	_ = readCloser.Close()
}

//将相关文件传入docker内,构建运行环境
func (ci *ContainerInstance) BuildDockerRunEnv(ctx context.Context, args *json_def.CompileAndRunArgs) {
	ci.StartContainer(ctx)
	//创建临时目录
	randStr := utils.GenRandomStr(20)
	absolutePath := fmt.Sprintf("%s/%s", DOCKERWORKPATH, randStr)
	mkdirStr := fmt.Sprintf("mkdir -p %s", absolutePath)
	var cmdList = []string{
		mkdirStr,
	}
	_ = ci.RunCmdInContainer(ctx, cmdList)

	//将文件拷贝到docker内去
	ci.CopyFileToContainer(ctx, args.SourceFile, absolutePath)
	ci.CopyFileToContainer(ctx, args.SysInputFile, absolutePath)

	//执行相关命令
	chmodStr := fmt.Sprintf("chown root:root -R %s", absolutePath)

	executableName := fmt.Sprintf("%s/%s", DOCKERWORKPATH, DOCKEREXENAME) //runner_FSM 所在位置
	userExeName := fmt.Sprintf("%s/%s_%s", absolutePath, randStr, args.Language)
	userOutputFile := fmt.Sprintf("%s/%s.output", absolutePath, randStr)
	getFileNameInHost := func(input string) string {
		stringSlice := strings.Split(input, "/")
		return stringSlice[len(stringSlice)-1]
	}
	sysInputFileInDocker := fmt.Sprintf("%s/%s", absolutePath, getFileNameInHost(args.SysInputFile))
	usrSourceFileInDocker := fmt.Sprintf("%s/%s", absolutePath, getFileNameInHost(args.SourceFile))
	exeRunnerStr := fmt.Sprintf("%s %s %s %d %d %d %s %s %s",
		executableName, args.Language, usrSourceFileInDocker,
		args.Time, args.Memory, args.Disk,
		userExeName, sysInputFileInDocker, userOutputFile)
	fmt.Printf("\nrunnerStr:%s\n", exeRunnerStr)
	cmdList = []string{
		chmodStr,     //修改文件owner
		exeRunnerStr, //执行docker内的判定程序
	}
	_ = ci.RunCmdInContainer(ctx, cmdList)
	defer ci.StopContainer(ctx)
}
