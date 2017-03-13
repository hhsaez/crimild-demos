require 'assets/prefabs/fire'

local camera = {
	type = 'crimild::Camera',
	enableCulling = false,
	transformation = {
		translate = { 0.0, 10.0, 10.0 },
	},
	components = {
		{
			type = 'crimild::FreeLookCameraComponent',
		},
	},
}

local environment = {
	filename = 'assets/models/room.obj',
}

scene = {
	type = 'crimild::Group',
	nodes = {
		environment,
		camera,

		fire( -10.0, 0.5, -10.0 ),
	},
}
