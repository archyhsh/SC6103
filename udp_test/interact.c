#include <stdio.h>
#include <string.h>

int main() {
    char command[50];
    printf("欢迎使用预约系统！\n");
    printf("请输入命令 (create_new / query_exist / exit): ");
    while (1) {
        printf("\n> ");   		  
        if (fgets(command, sizeof(command), stdin) == NULL) { 
            break;
        }
        command[strcspn(command, "\n")] = '\0';
        if (strcmp(command, "create_new") == 0) {
	       printf("请输入预约信息 (格式: ID 日期 时间):\n");
        }
        else if (strcmp(command, "query_exist") == 0) {
		  printf("请输入要查询的预约ID:\n");
		  // 查询后操作：1、修改时间段 2、删除预约 3、绑定推送
	   }
	   else if (strcmp(command, "exit") == 0) { 
	   	  printf("退出系统，再见！\n");
	   	  break;
	   }                                                                                                                                                                 
        else {
        	  printf("无效命令，请重试。\n");
	   }
    }
    return 0;
}