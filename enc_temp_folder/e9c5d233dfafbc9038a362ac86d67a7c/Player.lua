local Script = {}
Script.__index = Script

function Script.new(component, properties)
	local self = setmetatable({}, Script)
	self.component = component
	self.owner = component:GetOwner()
	return self
end

function Script:BeginPlay()

end

function Script:Tick(dt)
	local input = Engine.API.Input
	local dir = Vector(0, 0, 0)
	local speed = 1

	if input.IsRawKeyDown("W") then
		dir = self.owner:GetActorForwardVector()
	end
	if input.IsRawKeyDown("S") then
		dir = -self.owner:GetActorForwardVector()
	end
	if input.IsRawKeyDown("A") then
		dir = -self.owner:GetActorRightVector()
	end
	if input.IsRawKeyDown("D") then
		dir = self.owner:GetActorRightVector()
	end

	self.owner.Location = self.owner.Location + speed * dir * dt

end

function Script:EndPlay()

end

return Script