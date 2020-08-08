#include "robot.h"

#include <string.h>

#include <lfs.h>

#include "bsp/flash.h"

#define CONFIG_BASE_ADDRESS (ADDR_FLASH_END - sizeof(Robot_ID_t))

// variables used by the filesystem
static lfs_t lfs;
static lfs_file_t file;

static const PID_Params_t infantry_chassis_pid_array[4] = {{
		.kp = 0.5,
		.ki = 0.5,
		.kd = 0.5,
		.i_limit = 0.5,
		.out_limit = 0.5,
	},{
		.kp = 0.5,
		.ki = 0.5,
		.kd = 0.5,
		.i_limit = 0.5,
		.out_limit = 0.5,
	},{
		.kp = 0.5,
		.ki = 0.5,
		.kd = 0.5,
		.i_limit = 0.5,
		.out_limit = 0.5,
	},{
		.kp = 0.5,
		.ki = 0.5,
		.kd = 0.5,
		.i_limit = 0.5,
		.out_limit = 0.5,
	},
};

static const Robot_Config_t cfg_infantry = {
	.model = ROBOT_MODEL_INFANTRY,
	.param = {
		.chassis = {
			.motor_pid_param = infantry_chassis_pid_array,
			.follow_pid_param = {
				.kp = 0.5,
				.ki = 0.5,
				.kd = 0.5,
				.i_limit = 0.5,
				.out_limit = 0.5,
			},
			.low_pass_cutoff = 100.f,
		}, /* chassis */
		
		.gimbal = {
			.pid = {
				{
					/* GIMBAL_PID_YAW_IN */
					.kp = 0.5,
					.ki = 0.5,
					.kd = 0.5,
					.i_limit = 0.5,
					.out_limit = 0.5,
				},{
					/* GIMBAL_PID_YAW_OUT */
					.kp = 0.5,
					.ki = 0.5,
					.kd = 0.5,
					.i_limit = 0.5,
					.out_limit = 0.5,
				},{
					/* GIMBAL_PID_PIT_IN */
					.kp = 0.5,
					.ki = 0.5,
					.kd = 0.5,
					.i_limit = 0.5,
					.out_limit = 0.5,
				},{
					/* GIMBAL_PID_PIT_OUT */
					.kp = 0.5,
					.ki = 0.5,
					.kd = 0.5,
					.i_limit = 0.5,
					.out_limit = 0.5,
				},{
					/* GIMBAL_PID_REL_YAW */
					.kp = 0.5,
					.ki = 0.5,
					.kd = 0.5,
					.i_limit = 0.5,
					.out_limit = 0.5,
				},{
					/* GIMBAL_PID_REL_PIT, */
					.kp = 0.5,
					.ki = 0.5,
					.kd = 0.5,
					.i_limit = 0.5,
					.out_limit = 0.5,
				},
			},  /* pid */
			.low_pass_cutoff = 100.f,
		},  /* gimbal */
		
		.shoot = {
			.fric_pid_param = {
				{
					.kp = 0.5,
					.ki = 0.5,
					.kd = 0.5,
					.i_limit = 0.5,
					.out_limit = 0.5,
				},{
					.kp = 0.5,
					.ki = 0.5,
					.kd = 0.5,
					.i_limit = 0.5,
					.out_limit = 0.5,
				}
			},
			
			.trig_pid_param = {
				.kp = 0.5,
				.ki = 0.5,
				.kd = 0.5,
				.i_limit = 0.5,
				.out_limit = 0.5,
			},
			
			.low_pass_cutoff = {
				.fric = 100.f,
				.trig = 100.f,
			},
		},  /* shoot */
	},
}; /* cfg_infantry */

static const Robot_Config_t cfg_hero;
static const Robot_Config_t cfg_engineer;
static const Robot_Config_t cfg_drone;
static const Robot_Config_t cfg_sentry;

static const Robot_PilotConfig_t user_qs = {
	.param = {
		.cmd = {
			.sens_mouse = 0.5f,
			.sens_rc = 0.5f,
		},
	},
};

static int Robot_Flash_Read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size) {
	BSP_Flash_ReadBytes((ADDR_FLASH_SECTOR_8 + c->block_size * block + off), buffer, size);
	return LFS_ERR_OK;
}

static int Robot_Flash_Prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size) {
	BSP_Flash_WriteBytes((ADDR_FLASH_SECTOR_8 + c->block_size * block + off), buffer, size);
	return LFS_ERR_OK;
}

static int Robot_Flash_Erase(const struct lfs_config *c, lfs_block_t block) {
	(void)c;
	BSP_Flash_EraseSector(8 + block);
	return LFS_ERR_OK;
}

static int Robot_Flash_Sync(const struct lfs_config *c) {
	(void)c;
	return LFS_ERR_OK;
}

static const struct lfs_config cfg = {
	// block device operations
	.read  = Robot_Flash_Read,
	.prog  = Robot_Flash_Prog,
	.erase = Robot_Flash_Erase,
	.sync  = Robot_Flash_Sync,

	// block device configuration
	.read_size = 4,
	.prog_size = 4,
	.block_size = 128 * 1024,
	.block_count = 4,
	.cache_size = 16,
	.lookahead_size = 16,
	.block_cycles = 500,
};

void Robot_GetRobotID(Robot_ID_t *id) {
	#if 0
    // mount the filesystem
    int err = lfs_mount(&lfs, &cfg);

    // reformat if we can't mount the filesystem
    // this should only happen on the first boot
    if (err) {
        err = lfs_format(&lfs, &cfg);
        err = lfs_mount(&lfs, &cfg);
    }
	if (!err) {
		err = lfs_file_open(&lfs, &file, "robot_id", LFS_O_RDWR | LFS_O_CREAT);
		err = lfs_file_read(&lfs, &file, &id, sizeof(*id));
		
		// remember the storage is not updated until the file is closed successfully
		err = lfs_file_close(&lfs, &file);

		// release any resources we were using
		err = lfs_unmount(&lfs);
	}
	#endif
	BSP_Flash_ReadBytes(CONFIG_BASE_ADDRESS, (uint8_t*)id, sizeof(Robot_ID_t));
}

void Robot_SetRobotID(Robot_ID_t *id) {
	#if 0
    // mount the filesystem
    int err = lfs_mount(&lfs, &cfg);

    // reformat if we can't mount the filesystem
    // this should only happen on the first boot
    if (err) {
        err = lfs_format(&lfs, &cfg);
        err = lfs_mount(&lfs, &cfg);
    }
	if (!err) {
		err = lfs_file_open(&lfs, &file, "robot_id", LFS_O_RDWR | LFS_O_CREAT);
		err = lfs_file_write(&lfs, &file, &id, sizeof(*id));
		
		// remember the storage is not updated until the file is closed successfully
		err = lfs_file_close(&lfs, &file);

		// release any resources we were using
		err = lfs_unmount(&lfs);
	}
	#endif
	BSP_Flash_EraseSector(11);
	BSP_Flash_WriteBytes(CONFIG_BASE_ADDRESS, (uint8_t*)id, sizeof(Robot_ID_t));
}

const Robot_Config_t *Robot_GetConfig(Robot_Model_t model) {
	switch (model) {
		case ROBOT_MODEL_INFANTRY:
			return &cfg_infantry;
		case ROBOT_MODEL_HERO:
			return &cfg_hero;
		case ROBOT_MODEL_ENGINEER:
			return &cfg_engineer;
		case ROBOT_MODEL_DRONE:
			return &cfg_drone;
		case ROBOT_MODEL_SENTRY:
			return &cfg_sentry;
		case ROBOT_MODEL_NUM:
			/* Default infantry*/
			return &cfg_infantry;
	}
	return &cfg_infantry;
}

const Robot_PilotConfig_t *Robot_GetPilotConfig(Robot_Pilot_t pilot) {
	switch (pilot) {
		case ROBOT_PILOT_QS:
			return &user_qs;
		case ROBOT_PILOT_NUM:
			/* Default user_qs*/
			return &user_qs;
	}
	return &user_qs;
}

static const struct {
	Robot_Model_t model;
	const char* name;
} model_string_map[] = {
	{ROBOT_MODEL_INFANTRY, "Infantry"},
	{ROBOT_MODEL_HERO, "Hero"},
	{ROBOT_MODEL_ENGINEER, "Engineer"},
	{ROBOT_MODEL_DRONE, "Drone"},
	{ROBOT_MODEL_SENTRY, "Sentry"},
	{ROBOT_MODEL_NUM, NULL},
};

static const struct {
	Robot_Pilot_t pilot;
	const char* name;
} pilot_string_map[] = {
	{ROBOT_PILOT_QS, "qs"},
	{ROBOT_PILOT_NUM, NULL},
};

Robot_Model_t Robot_GetModelByName(const char *name) {
    for (int j = 0; model_string_map[j].name != NULL; j++) {
        if (strstr(model_string_map[j].name, name) != NULL) {
            return model_string_map[j].model;
        }
    }
    return ROBOT_MODEL_NUM; /* No match. */
}

Robot_Pilot_t Robot_GetPilotByName(const char *name) {
    for (int j = 0; pilot_string_map[j].name != NULL; j++) {
        if (strcmp(pilot_string_map[j].name, name) == 0) {
            return pilot_string_map[j].pilot;
        }
    }
    return ROBOT_PILOT_NUM; /* No match. */
}

const char *Robot_GetNameByModel(Robot_Model_t model) {
    for (int j = 0; model_string_map[j].name != NULL; j++) {
        if (model_string_map[j].model == model) {
            return model_string_map[j].name;
        }
    }
    return "Unknown"; /* No match. */
}

const char *Robot_GetNameByPilot(Robot_Pilot_t pilot) {
    for (int j = 0; pilot_string_map[j].name != NULL; j++) {
        if (pilot_string_map[j].pilot == pilot) {
            return pilot_string_map[j].name;
        }
    }
    return "Unknown"; /* No match. */
}
