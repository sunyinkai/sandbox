package container_manage

type ContainerEntity struct {
	contId     string
	isOccupied bool
}

func (ce *ContainerEntity) Acquire() {
	ce.isOccupied = true
}

func (ce *ContainerEntity) Release() {
	ce.isOccupied = false
}

func (ce *ContainerEntity) IsBusy() bool {
	return ce.isOccupied == true
}

type ContainerPool struct {
	containerList []ContainerEntity
}

func (cp *ContainerPool) Init(size int) {
	cp.containerList = make([]ContainerEntity, size)

}
