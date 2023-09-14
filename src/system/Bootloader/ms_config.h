/* 最大命令长度 */
#define MS_MAX_CMD_LENGTH (64)

/* 命令参数上限 */
#define MS_MAX_ARG_NUM (5)

/* 历史命令数量 */
#define MS_MAX_HISTORY_NUM (4)

/* 命令行打印缓冲区长度 */
#define MS_WIRITE_BUFF_SIZE (128)

/* cat命令缓冲区 */
#define MS_CAT_BUFF_SIZE (128)

/* 自定义颜色 */
#define MS_HEAD_COLOR MS_COLOR_GREEN

/* 系统名称 */
#define MS_OS_NAME "XRobot"

#define _MS_STR_2(_arg) #_arg
#define _MS_STR_1(_arg) _MS_STR_2(_arg)

/* 用户名称 */
#define MS_USER_NAME _MS_STR_1(XROBOT_BOARD)

/* 欢迎信息 */
#define MS_HELLO_MESSAGE "Welcome to use XRobot!"

/* 登陆命令 */
#define MS_INIT_COMMAND ""

/* 内置文件选择编译 */
#define MS_FILE_TTY (0)

#define MS_FILE_README (0)

/* 内置命令选择编译 */
#define MS_CMD_PWD (0)

#define MS_CMD_LS (0)

#define MS_CMD_CD (0)

#define MS_CMD_CAT (0)

#define MS_CMD_ECHO (0)

#define MS_CMD_CLEAR (0)
