#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include <locale.h>
#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#endif
// 常量定义
#define MAX_COURSES 50
#define MAX_TASKS 50
#define MAX_MOOD 365
#define MAX_ERRORS 100
#define MAX_LINE_LENGTH 1024

// 数据结构定义
// 学生信息结构体
typedef struct {
	char name[100];
	char major[50];
	int grade;
	char learning_baseline[100]; // 学习基础
	char learning_goal[100]; // 学习目标
} StudentInfo;

typedef struct {
	char course_name[100];
	char time[50];
	char classroom[50];
	int day; // 0=周一, 1=周二...
} Course;

typedef struct {
	char task_name[100];
	char deadline[20];
	int completed;
	char description[200];
	int duration; // 完成该任务所需时间（分钟）
} Task;

typedef struct {
	char date[20];
	char status[20]; // "高效", "突击", "放松"
} MoodLog;

typedef struct {
	char question_path[200];
	char subject[50];
	char difficulty[20];
} ErrorQuestion;

// 全局变量
StudentInfo current_student = {"", "", 0, "", ""};
Course courses[MAX_COURSES];
int course_count = 0;
Task tasks[MAX_TASKS];
int task_count = 0;
MoodLog mood_logs[MAX_MOOD];
int mood_count = 0;
ErrorQuestion error_questions[MAX_ERRORS];
int error_count = 0;

GtkWidget *main_window;
GtkWidget *stack;
gboolean dark_mode = FALSE;
GtkWidget *student_info_page;
GtkWidget *course_page;
GtkWidget *task_page;
GtkWidget *mood_page;
GtkWidget *chart_page;
GtkWidget *error_page;
GtkWidget *profile_page;
GtkWidget *time_label; // 系统时间显示标签

// 个人中心页面显示标签（用于动态更新）
GtkWidget *profile_name_label;
GtkWidget *profile_major_label;
GtkWidget *profile_grade_label;
GtkWidget *profile_baseline_label;
GtkWidget *profile_goal_label;
// 学生信息页面控件引用（用于动态更新）
GtkWidget *student_name_entry = NULL;
GtkWidget *student_major_entry = NULL;
GtkWidget *student_grade_entry = NULL;
GtkWidget *student_baseline_combo = NULL;
GtkWidget *student_goal_combo = NULL;


// 用于数据更新的全局控件引用（避免重复创建）
GtkListStore *course_store = NULL;
GtkListStore *task_store = NULL;
GtkListStore *mood_store = NULL;
GtkListStore *error_store = NULL;
GtkWidget *chart_area = NULL;
GtkWidget *error_subject_combo = NULL; // 错题本科目选择框
gboolean error_subject_needs_update = FALSE; // 标记是否需要更新错题本科目列表

// 函数声明
void save_data();
void load_data();
void create_main_window(GtkApplication *app, gpointer user_data);
gboolean on_main_window_delete(GtkWidget *widget, GdkEvent *event, gpointer user_data);
void show_student_info_page();
void show_course_page();
void show_task_page();
void show_mood_page();
void show_chart_page();
void show_error_page();
void show_profile_page();

// 数据更新函数（避免重建控件）
void update_course_store();
void update_task_store();
void update_mood_store();
void update_error_store();
void update_student_info_display();
void update_profile_display();
void refresh_chart();
void refresh_course_page();
void show_help_callback(GtkWidget *widget, gpointer data);
void view_error_question_callback(GtkWidget *widget, gpointer data);
void delete_error_question_callback(GtkWidget *widget, gpointer data);
void recommend_task_callback(GtkWidget *widget, gpointer data);
void update_error_subject_combo();
void toggle_dark_mode(GtkWidget *widget, gpointer data);
char *get_data_file_path(const char *filename);

// 数据更新函数实现

void update_course_store() {
	if (!course_store)
		return;

	gtk_list_store_clear(course_store);

	const char *days[] = {"周一", "周二", "周三", "周四", "周五", "周六", "周日"};

	for (int i = 0; i < course_count; i++) {
		GtkTreeIter iter;
		gtk_list_store_append(course_store, &iter);
		gtk_list_store_set(course_store, &iter,
		                   0, courses[i].course_name,
		                   1, courses[i].time,
		                   2, courses[i].classroom,
		                   3, days[courses[i].day],
		                   -1);
	}
}

// 刷新课程表视图
void refresh_course_page() {
	if (!course_page)
		return;

	// 查找课程表格frame
	GList *children = gtk_container_get_children(GTK_CONTAINER(course_page));
	GtkWidget *table_frame = NULL;

	for (GList *iter = children; iter != NULL; iter = g_list_next(iter)) {
		GtkWidget *child = GTK_WIDGET(iter->data);
		// 检查是否是frame且标题包含"课程安排"
		if (GTK_IS_FRAME(child)) {
			const char *label = gtk_frame_get_label(GTK_FRAME(child));
			if (label && strstr(label, "课程安排")) {
				table_frame = child;
				break;
			}
		}
	}
	g_list_free(children);

	if (!table_frame)
		return;

	// 获取table_grid
	GtkWidget *table_grid = gtk_bin_get_child(GTK_BIN(table_frame));
	if (!table_grid)
		return;

	// 清空表格
	gtk_container_foreach(GTK_CONTAINER(table_grid), (GtkCallback)gtk_widget_destroy, NULL);

	// 重新添加星期标题行
	const char *days[] = {"周一", "周二", "周三", "周四", "周五", "周六", "周日"};
	GtkWidget *time_header = gtk_label_new("时间");
	gtk_widget_set_size_request(time_header, 80, -1);
	gtk_grid_attach(GTK_GRID(table_grid), time_header, 0, 0, 1, 1);

	for (int i = 0; i < 7; i++) {
		GtkWidget *day_label = gtk_label_new(NULL);
		char markup[50];
		snprintf(markup, sizeof(markup), "<b>%s</b>", days[i]);
		gtk_label_set_markup(GTK_LABEL(day_label), markup);
		gtk_widget_set_size_request(day_label, 120, -1);
		gtk_grid_attach(GTK_GRID(table_grid), day_label, i + 1, 0, 1, 1);
	}

	// 重新添加每天的课程
	GtkWidget *day_columns[7];
	for (int day = 0; day < 7; day++) {
		day_columns[day] = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
		gtk_widget_set_size_request(day_columns[day], 120, -1);
		gtk_grid_attach(GTK_GRID(table_grid), day_columns[day], day + 1, 1, 1, 1);

		int day_course_count = 0;
		for (int i = 0; i < course_count; i++) {
			if (courses[i].day == day) {
				day_course_count++;
			}
		}

		if (day_course_count == 0) {
			GtkWidget *empty_label = gtk_label_new("暂无课程");
			gtk_label_set_justify(GTK_LABEL(empty_label), GTK_JUSTIFY_CENTER);
			gtk_box_pack_start(GTK_BOX(day_columns[day]), empty_label, TRUE, TRUE, 0);
		} else {
			for (int i = 0; i < course_count; i++) {
				if (courses[i].day == day) {
					// 创建课程卡片
					GtkWidget *course_card = gtk_frame_new(NULL);
					{ GtkStyleContext *ctx_cc = gtk_widget_get_style_context(course_card); gtk_style_context_add_class(ctx_cc, "course-card"); }

					GtkWidget *course_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
					gtk_widget_set_margin_start(course_box, 6);
					gtk_widget_set_margin_end(course_box, 6);
					gtk_widget_set_margin_top(course_box, 5);
					gtk_widget_set_margin_bottom(course_box, 5);
					gtk_container_add(GTK_CONTAINER(course_card), course_box);

					// 课程名称
					GtkWidget *name_label = gtk_label_new(courses[i].course_name);
					{ GtkStyleContext *ctx_nl = gtk_widget_get_style_context(name_label); gtk_style_context_add_class(ctx_nl, "course-card-name"); }
					gtk_label_set_line_wrap(GTK_LABEL(name_label), TRUE);
					gtk_label_set_justify(GTK_LABEL(name_label), GTK_JUSTIFY_CENTER);
					gtk_box_pack_start(GTK_BOX(course_box), name_label, FALSE, FALSE, 0);

					// 时间
					GtkWidget *time_label = gtk_label_new(courses[i].time);
					{ GtkStyleContext *ctx_tl = gtk_widget_get_style_context(time_label); gtk_style_context_add_class(ctx_tl, "course-card-time"); }
					gtk_label_set_justify(GTK_LABEL(time_label), GTK_JUSTIFY_CENTER);
					gtk_widget_set_size_request(time_label, 110, -1);
					gtk_box_pack_start(GTK_BOX(course_box), time_label, FALSE, FALSE, 0);

					// 教室
					if (strlen(courses[i].classroom) > 0) {
						GtkWidget *room_label = gtk_label_new(courses[i].classroom);
						{ GtkStyleContext *ctx_rl = gtk_widget_get_style_context(room_label); gtk_style_context_add_class(ctx_rl, "course-card-room"); }
						gtk_label_set_justify(GTK_LABEL(room_label), GTK_JUSTIFY_CENTER);
						gtk_box_pack_start(GTK_BOX(course_box), room_label, FALSE, FALSE, 0);
					}

					gtk_box_pack_start(GTK_BOX(day_columns[day]), course_card, FALSE, FALSE, 2);
				}
			}
		}
	}

	// 强制刷新整个课程页面
	gtk_widget_hide(course_page);
	gtk_widget_show_all(course_page);

	// 处理GTK事件循环确保刷新完成
	while (gtk_events_pending()) {
		gtk_main_iteration_do(FALSE);
	}
}

// 更新错题本科目选择列表
void update_error_subject_combo() {
	if (!error_subject_combo)
		return;

	// 重置更新标志
	error_subject_needs_update = FALSE;

	// 获取当前选中的科目索引（用于恢复选择）
	gint current_index = gtk_combo_box_get_active(GTK_COMBO_BOX(error_subject_combo));

	// 清空所有选项
	gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(error_subject_combo));

	// 如果课程表为空，添加默认选项
	if (course_count == 0) {
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(error_subject_combo), "暂无课程，请先添加课程");
		gtk_combo_box_set_active(GTK_COMBO_BOX(error_subject_combo), 0);
	} else {
		// 添加课程表中的所有课程（去重）
		for (int i = 0; i < course_count; i++) {
			// 检查是否已添加过相同课程名
			int dup = 0;
			for (int j = 0; j < i; j++) {
				if (strcmp(courses[i].course_name, courses[j].course_name) == 0) {
					dup = 1;
					break;
				}
			}
			if (!dup)
				gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(error_subject_combo), courses[i].course_name);
		}

		// 尝试恢复之前的选择（使用索引更可靠）
		if (current_index >= 0 && current_index < course_count) {
			gtk_combo_box_set_active(GTK_COMBO_BOX(error_subject_combo), current_index);
		} else {
			gtk_combo_box_set_active(GTK_COMBO_BOX(error_subject_combo), 0);
		}
	}

	// 强制刷新显示 - 使用更强大的刷新方法
	gtk_widget_hide(error_subject_combo);
	gtk_widget_show(error_subject_combo);

	// 处理GTK事件循环以确保刷新生效
	while (gtk_events_pending()) {
		gtk_main_iteration_do(FALSE);
	}

	// 额外的刷新步骤确保下拉框弹出菜单也被更新
	// g_object_refresh 不存在于 GTK3，已移除
}

// 请求更新错题本科目列表（即使控件还未创建）
void request_error_subject_update() {
	error_subject_needs_update = TRUE;
	// 如果控件已存在，立即更新
	if (error_subject_combo) {
		update_error_subject_combo();
	}

	// 如果错题页面当前正在显示，强制刷新整个页面
	if (error_page != NULL && gtk_widget_get_visible(error_page)) {
		update_error_store();
	}
}

void update_task_store() {
	if (!task_store)
		return;

	gtk_list_store_clear(task_store);

	for (int i = 0; i < task_count; i++) {
		GtkTreeIter iter;
		gtk_list_store_append(task_store, &iter);
		char duration_str[20];
		snprintf(duration_str, sizeof(duration_str), "%d分钟", tasks[i].duration);
		gtk_list_store_set(task_store, &iter,
		                   0, tasks[i].task_name,
		                   1, tasks[i].deadline,
		                   2, tasks[i].description,
		                   3, tasks[i].completed,
		                   4, duration_str,
		                   -1);
	}
}

void update_mood_store() {
	if (!mood_store)
		return;

	gtk_list_store_clear(mood_store);

	for (int i = 0; i < mood_count; i++) {
		GtkTreeIter iter;
		gtk_list_store_append(mood_store, &iter);
		gtk_list_store_set(mood_store, &iter,
		                   0, mood_logs[i].date,
		                   1, mood_logs[i].status,
		                   -1);
	}
}

void update_error_store() {
	if (!error_store)
		return;

	gtk_list_store_clear(error_store);

	for (int i = 0; i < error_count; i++) {
		GtkTreeIter iter;
		gtk_list_store_append(error_store, &iter);
		gtk_list_store_set(error_store, &iter,
		                   0, error_questions[i].question_path,
		                   1, error_questions[i].subject,
		                   2, error_questions[i].difficulty,
		                   -1);
	}
}

// 更新个人中心页面显示
void update_profile_display() {
	if (!profile_name_label)
		return;

	// 更新姓名
	gtk_label_set_text(GTK_LABEL(profile_name_label),
	                   current_student.name[0] ? current_student.name : "未填写");

	// 更新专业
	gtk_label_set_text(GTK_LABEL(profile_major_label),
	                   current_student.major[0] ? current_student.major : "未填写");

	// 更新年级
	char grade_str[20];
	snprintf(grade_str, sizeof(grade_str), "%d", current_student.grade);
	gtk_label_set_text(GTK_LABEL(profile_grade_label),
	                   current_student.grade ? grade_str : "未填写");

	// 更新学习基础
	gtk_label_set_text(GTK_LABEL(profile_baseline_label),
	                   current_student.learning_baseline[0] ? current_student.learning_baseline : "未填写");

	// 更新学习目标
	gtk_label_set_text(GTK_LABEL(profile_goal_label),
	                   current_student.learning_goal[0] ? current_student.learning_goal : "未填写");
}

// 更新学生信息页面显示（页面已存在时刷新entry控件）
void update_student_info_display() {
	if (!student_info_page)
		return;

	if (student_name_entry)
		gtk_entry_set_text(GTK_ENTRY(student_name_entry),
		                   current_student.name[0] ? current_student.name : "");

	if (student_major_entry)
		gtk_entry_set_text(GTK_ENTRY(student_major_entry),
		                   current_student.major[0] ? current_student.major : "");

	if (student_grade_entry) {
		char grade_str[20];
		snprintf(grade_str, sizeof(grade_str), "%d", current_student.grade);
		gtk_entry_set_text(GTK_ENTRY(student_grade_entry), grade_str);
	}

	if (student_baseline_combo) {
		const char *baseline = current_student.learning_baseline;
		const char *items[] = {"薄弱", "一般", "良好", "优秀"};
		int found = 0;
		int i;
		for (i = 0; i < 4; i++) {
			if (strcmp(baseline, items[i]) == 0) {
				gtk_combo_box_set_active(GTK_COMBO_BOX(student_baseline_combo), i);
				found = 1;
				break;
			}
		}
		if (!found)
			gtk_combo_box_set_active(GTK_COMBO_BOX(student_baseline_combo), 0);
	}

	if (student_goal_combo) {
		const char *goal = current_student.learning_goal;
		const char *items[] = {"考研", "就业", "出国", "考公", "其他"};
		int found = 0;
		int i;
		for (i = 0; i < 5; i++) {
			if (strcmp(goal, items[i]) == 0) {
				gtk_combo_box_set_active(GTK_COMBO_BOX(student_goal_combo), i);
				found = 1;
				break;
			}
		}
		if (!found)
			gtk_combo_box_set_active(GTK_COMBO_BOX(student_goal_combo), 0);
	}
}

void refresh_chart() {
	if (chart_area) {
		gtk_widget_queue_draw(chart_area);
	}
}

// 保存数据到文件
void save_data() {
	FILE *file = fopen(get_data_file_path("study_data.txt"), "w");
	if (!file) {
		#ifdef _WIN32
		// 尝试创建AppData目录
		char dir_path[512];
		snprintf(dir_path, sizeof(dir_path), "%s", get_data_file_path(""));
		CreateDirectoryA(dir_path, NULL);
		#endif
		file = fopen(get_data_file_path("study_data.txt"), "w");
		if (!file)
			return;
	}

	fprintf(file, "%s\n%s\n%d\n%s\n%s\n",
	        current_student.name,
	        current_student.major,
	        current_student.grade,
	        current_student.learning_baseline,
	        current_student.learning_goal);

	fprintf(file, "%d\n", course_count);
	for (int i = 0; i < course_count; i++) {
		fprintf(file, "%s\n%s\n%s\n%d\n",
		        courses[i].course_name,
		        courses[i].time,
		        courses[i].classroom,
		        courses[i].day);
	}

	fprintf(file, "%d\n", task_count);
	for (int i = 0; i < task_count; i++) {
		fprintf(file, "%s\n%s\n%d\n%s\n%d\n",
		        tasks[i].task_name,
		        tasks[i].deadline,
		        tasks[i].completed,
		        tasks[i].description,
		        tasks[i].duration);
	}

	fprintf(file, "%d\n", mood_count);
	for (int i = 0; i < mood_count; i++) {
		fprintf(file, "%s\n%s\n",
		        mood_logs[i].date,
		        mood_logs[i].status);
	}

	fprintf(file, "%d\n", error_count);
	for (int i = 0; i < error_count; i++) {
		fprintf(file, "%s\n%s\n%s\n",
		        error_questions[i].question_path,
		        error_questions[i].subject,
		        error_questions[i].difficulty);
	}

	fclose(file);
}

// 导入课程回调
void import_course_callback(GtkWidget *widget, gpointer data) {
	(void)widget;
	(void)data; // 消除未使用参数警告
	GtkWidget *dialog = gtk_file_chooser_dialog_new("选择课程CSV文件",
	                    GTK_WINDOW(main_window),
	                    GTK_FILE_CHOOSER_ACTION_OPEN,
	                    "_取消", GTK_RESPONSE_CANCEL,
	                    "_确定", GTK_RESPONSE_ACCEPT,
	                    NULL);

	GtkFileFilter *filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "CSV文件");
	gtk_file_filter_add_pattern(filter, "*.csv");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

		// 检查filename是否为NULL
		if (!filename) {
			gtk_widget_destroy(dialog);
			return;
		}

		FILE *file = fopen(filename, "r");

		if (file) {
			char line[MAX_LINE_LENGTH];
			const char *days[] = {"周一", "周二", "周三", "周四", "周五", "周六", "周日"};
			int imported_count = 0;

			// 跳过可能的CSV表头行
			fgets(line, sizeof(line), file);

			while (fgets(line, sizeof(line), file)) {
				// 跳过空行
				if (line[0] == '\n' || line[0] == '\r')
					continue;

				// 创建临时副本用于解析，避免修改原始数据
				char line_copy[MAX_LINE_LENGTH];
				strncpy(line_copy, line, sizeof(line_copy) - 1);
				line_copy[sizeof(line_copy) - 1] = '\0';

				// 在CSV中，使用strtok分割带引号的字段
				char *course_name = strtok(line_copy, ",");
				char *time = strtok(NULL, ",");
				char *classroom = strtok(NULL, ",");
				char *day_str = strtok(NULL, "\n\r");

				if (course_name && time && classroom && day_str) {
					// 去除可能的引号
					if (course_name[0] == '"') {
						course_name++;
						char *end = strrchr(course_name, '"');
						if (end)
							*end = '\0';
					}

					if (time[0] == '"') {
						time++;
						char *end = strrchr(time, '"');
						if (end)
							*end = '\0';
					}

					if (classroom[0] == '"') {
						classroom++;
						char *end = strrchr(classroom, '"');
						if (end)
							*end = '\0';
					}

					if (day_str[0] == '"') {
						day_str++;
						char *end = strrchr(day_str, '"');
						if (end)
							*end = '\0';
					}

					int day = -1;
					for (int i = 0; i < 7; i++) {
						if (strcmp(day_str, days[i]) == 0) {
							day = i;
							break;
						}
					}

					if (day == -1)
						continue; // 无效数据

					// 检查是否已存在相同课程名，避免重复导入
					int duplicate = 0;
					for (int j = 0; j < course_count; j++) {
						if (strcmp(courses[j].course_name, course_name) == 0) {
							duplicate = 1;
							break;
						}
					}
					if (duplicate) continue;

					// 边界检查
					if (course_count < MAX_COURSES) {
						strncpy(courses[course_count].course_name, course_name, sizeof(courses[course_count].course_name) - 1);
						courses[course_count].course_name[sizeof(courses[course_count].course_name) - 1] = '\0';

						strncpy(courses[course_count].time, time, sizeof(courses[course_count].time) - 1);
						courses[course_count].time[sizeof(courses[course_count].time) - 1] = '\0';

						strncpy(courses[course_count].classroom, classroom, sizeof(courses[course_count].classroom) - 1);
						courses[course_count].classroom[sizeof(courses[course_count].classroom) - 1] = '\0';

						courses[course_count].day = day;
						course_count++;
						imported_count++;
					}
				}
			}
			fclose(file);
			save_data();

			char msg[100];
			snprintf(msg, sizeof(msg), "成功导入 %d 门课程！", imported_count);
			GtkWidget *info_dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
			                         GTK_DIALOG_MODAL,
			                         GTK_MESSAGE_INFO,
			                         GTK_BUTTONS_OK,
			                         msg);
			gtk_dialog_run(GTK_DIALOG(info_dialog));
			gtk_widget_destroy(info_dialog);

			refresh_course_page();
			update_error_subject_combo();
		} else {
			GtkWidget *error_dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
			                          GTK_DIALOG_MODAL,
			                          GTK_MESSAGE_ERROR,
			                          GTK_BUTTONS_OK,
			                          "无法打开文件！");
			gtk_dialog_run(GTK_DIALOG(error_dialog));
			gtk_widget_destroy(error_dialog);
		}
		g_free(filename); // 将g_free移到if块内部
	}
	gtk_widget_destroy(dialog);
}

// 导出课程
void export_course_callback(GtkWidget *widget, gpointer data) {
	(void)widget;
	(void)data; // 消除未使用参数警告
	GtkWidget *dialog = gtk_file_chooser_dialog_new("导出课程CSV文件",
	                    GTK_WINDOW(main_window),
	                    GTK_FILE_CHOOSER_ACTION_SAVE,
	                    "_取消", GTK_RESPONSE_CANCEL,
	                    "_保存", GTK_RESPONSE_ACCEPT,
	                    NULL);

	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

		// 检查filename是否为NULL
		if (!filename) {
			gtk_widget_destroy(dialog);
			return;
		}

		FILE *file = fopen(filename, "w");
		if (file) {

			const char *days[] = {"周一", "周二", "周三", "周四", "周五", "周六", "周日"};

			for (int i = 0; i < course_count; i++) {
				fprintf(file, "\"%s\",\"%s\",\"%s\",\"%s\"\n",
				        courses[i].course_name,
				        courses[i].time,
				        courses[i].classroom,
				        days[courses[i].day]);
			}
			fclose(file);

			GtkWidget *info_dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
			                         GTK_DIALOG_MODAL,
			                         GTK_MESSAGE_INFO,
			                         GTK_BUTTONS_OK,
			                         "课程导出成功！");
			gtk_dialog_run(GTK_DIALOG(info_dialog));
			gtk_widget_destroy(info_dialog);

		} else {
			GtkWidget *error_dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
			                          GTK_DIALOG_MODAL,
			                          GTK_MESSAGE_ERROR,
			                          GTK_BUTTONS_OK,
			                          "无法写入文件！");
			gtk_dialog_run(GTK_DIALOG(error_dialog));
			gtk_widget_destroy(error_dialog);
		}
		g_free(filename); // 将g_free移到if块内部
	}
	gtk_widget_destroy(dialog);
}

// 从文件加载数据
void load_data() {
	FILE *file = fopen(get_data_file_path("study_data.txt"), "r");
	if (!file)
		return;

	// 安全读取学生信息
	char temp_name[100], temp_major[50], temp_baseline[100], temp_goal[100];
	int temp_grade;

	if (fscanf(file, "%99s", temp_name) == 1) {
		strncpy(current_student.name, temp_name, sizeof(current_student.name) - 1);
		current_student.name[sizeof(current_student.name) - 1] = '\0';
	}

	if (fscanf(file, "%49s", temp_major) == 1) {
		strncpy(current_student.major, temp_major, sizeof(current_student.major) - 1);
		current_student.major[sizeof(current_student.major) - 1] = '\0';
	}

	if (fscanf(file, "%d", &temp_grade) == 1) {
		current_student.grade = temp_grade;
	}

	if (fscanf(file, "%99s", temp_baseline) == 1) {
		strncpy(current_student.learning_baseline, temp_baseline, sizeof(current_student.learning_baseline) - 1);
		current_student.learning_baseline[sizeof(current_student.learning_baseline) - 1] = '\0';
	} else {
		strncpy(current_student.learning_baseline, "一般", sizeof(current_student.learning_baseline) - 1);
		current_student.learning_baseline[sizeof(current_student.learning_baseline) - 1] = '\0';
	}

	if (fscanf(file, "%99s", temp_goal) == 1) {
		strncpy(current_student.learning_goal, temp_goal, sizeof(current_student.learning_goal) - 1);
		current_student.learning_goal[sizeof(current_student.learning_goal) - 1] = '\0';
	} else {
		strncpy(current_student.learning_goal, "其他", sizeof(current_student.learning_goal) - 1);
		current_student.learning_goal[sizeof(current_student.learning_goal) - 1] = '\0';
	}

	// 读取课程
	int temp_course_count;
	if (fscanf(file, "%d", &temp_course_count) == 1) {
		temp_course_count = temp_course_count > MAX_COURSES ? MAX_COURSES : temp_course_count;

		for (int i = 0; i < temp_course_count; i++) {
			char temp_course[100], temp_time[50], temp_classroom[50];
			int temp_day;

			if (fscanf(file, "%99s", temp_course) == 1) {
				strncpy(courses[i].course_name, temp_course, sizeof(courses[i].course_name) - 1);
				courses[i].course_name[sizeof(courses[i].course_name) - 1] = '\0';
			}

			if (fscanf(file, "%49s", temp_time) == 1) {
				strncpy(courses[i].time, temp_time, sizeof(courses[i].time) - 1);
				courses[i].time[sizeof(courses[i].time) - 1] = '\0';
			}

			if (fscanf(file, "%49s", temp_classroom) == 1) {
				strncpy(courses[i].classroom, temp_classroom, sizeof(courses[i].classroom) - 1);
				courses[i].classroom[sizeof(courses[i].classroom) - 1] = '\0';
			}

			if (fscanf(file, "%d", &temp_day) == 1) {
				courses[i].day = temp_day;
			}
		}
		course_count = temp_course_count;
	}

	// 读取任务
	int temp_task_count;
	if (fscanf(file, "%d", &temp_task_count) == 1) {
		temp_task_count = temp_task_count > MAX_TASKS ? MAX_TASKS : temp_task_count;

		for (int i = 0; i < temp_task_count; i++) {
			char temp_task[100], temp_deadline[20], temp_desc[200];
			int temp_completed, temp_duration;

			if (fscanf(file, "%99s", temp_task) == 1) {
				strncpy(tasks[i].task_name, temp_task, sizeof(tasks[i].task_name) - 1);
				tasks[i].task_name[sizeof(tasks[i].task_name) - 1] = '\0';
			}

			if (fscanf(file, "%19s", temp_deadline) == 1) {
				strncpy(tasks[i].deadline, temp_deadline, sizeof(tasks[i].deadline) - 1);
				tasks[i].deadline[sizeof(tasks[i].deadline) - 1] = '\0';
			}

			if (fscanf(file, "%d", &temp_completed) == 1) {
				tasks[i].completed = temp_completed;
			}

			if (fscanf(file, "%199s", temp_desc) == 1) {
				strncpy(tasks[i].description, temp_desc, sizeof(tasks[i].description) - 1);
				tasks[i].description[sizeof(tasks[i].description) - 1] = '\0';
			}

			if (fscanf(file, "%d", &temp_duration) == 1) {
				tasks[i].duration = temp_duration;
			} else {
				tasks[i].duration = 30; // 默认30分钟
			}
		}
		task_count = temp_task_count;
	}

	// 读取心情记录
	int temp_mood_count;
	if (fscanf(file, "%d", &temp_mood_count) == 1) {
		temp_mood_count = temp_mood_count > MAX_MOOD ? MAX_MOOD : temp_mood_count;

		for (int i = 0; i < temp_mood_count; i++) {
			char temp_date[20], temp_status[20];

			if (fscanf(file, "%19s", temp_date) == 1) {
				strncpy(mood_logs[i].date, temp_date, sizeof(mood_logs[i].date) - 1);
				mood_logs[i].date[sizeof(mood_logs[i].date) - 1] = '\0';
			}

			if (fscanf(file, "%19s", temp_status) == 1) {
				strncpy(mood_logs[i].status, temp_status, sizeof(mood_logs[i].status) - 1);
				mood_logs[i].status[sizeof(mood_logs[i].status) - 1] = '\0';
			}
		}
		mood_count = temp_mood_count;
	}

	// 读取错题记录
	int temp_error_count;
	if (fscanf(file, "%d", &temp_error_count) == 1) {
		temp_error_count = temp_error_count > MAX_ERRORS ? MAX_ERRORS : temp_error_count;

		for (int i = 0; i < temp_error_count; i++) {
			char temp_path[200], temp_subject[50], temp_difficulty[20];

			if (fscanf(file, "%199s", temp_path) == 1) {
				strncpy(error_questions[i].question_path, temp_path, sizeof(error_questions[i].question_path) - 1);
				error_questions[i].question_path[sizeof(error_questions[i].question_path) - 1] = '\0';
			}

			if (fscanf(file, "%49s", temp_subject) == 1) {
				strncpy(error_questions[i].subject, temp_subject, sizeof(error_questions[i].subject) - 1);
				error_questions[i].subject[sizeof(error_questions[i].subject) - 1] = '\0';
			}

			if (fscanf(file, "%19s", temp_difficulty) == 1) {
				strncpy(error_questions[i].difficulty, temp_difficulty, sizeof(error_questions[i].difficulty) - 1);
				error_questions[i].difficulty[sizeof(error_questions[i].difficulty) - 1] = '\0';
			}
		}
		error_count = temp_error_count;
	}

	fclose(file);
}

// 更新学生信息回调函数
void update_student_info_callback(GtkWidget *widget, gpointer data) {
	(void)data; // 忽略未使用参数
	GtkWidget *name_entry = g_object_get_data(G_OBJECT(widget), "name_entry");
	GtkWidget *major_entry = g_object_get_data(G_OBJECT(widget), "major_entry");
	GtkWidget *grade_entry = g_object_get_data(G_OBJECT(widget), "grade_entry");
	GtkWidget *baseline_combo = g_object_get_data(G_OBJECT(widget), "baseline_combo");
	GtkWidget *goal_combo = g_object_get_data(G_OBJECT(widget), "goal_combo");

	const char *name = gtk_entry_get_text(GTK_ENTRY(name_entry));
	const char *major = gtk_entry_get_text(GTK_ENTRY(major_entry));
	gchar *baseline = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(baseline_combo));
	gchar *goal = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(goal_combo));

	// 输入验证
	if (name == NULL || strlen(name) == 0) {
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
		                    GTK_DIALOG_MODAL,
		                    GTK_MESSAGE_ERROR,
		                    GTK_BUTTONS_OK,
		                    "姓名不能为空！");
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		g_free(baseline);
		g_free(goal);
		return;
	}

	if (major == NULL || strlen(major) == 0) {
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
		                    GTK_DIALOG_MODAL,
		                    GTK_MESSAGE_ERROR,
		                    GTK_BUTTONS_OK,
		                    "专业不能为空！");
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		g_free(baseline);
		g_free(goal);
		return;
	}

	// 使用安全字符串复制
	strncpy(current_student.name, name, sizeof(current_student.name) - 1);
	current_student.name[sizeof(current_student.name) - 1] = '\0';

	strncpy(current_student.major, major, sizeof(current_student.major) - 1);
	current_student.major[sizeof(current_student.major) - 1] = '\0';

	current_student.grade = atoi(gtk_entry_get_text(GTK_ENTRY(grade_entry)));

	if (baseline) {
		strncpy(current_student.learning_baseline, baseline, sizeof(current_student.learning_baseline) - 1);
		current_student.learning_baseline[sizeof(current_student.learning_baseline) - 1] = '\0';
		g_free(baseline);
	} else {
		strncpy(current_student.learning_baseline, "一般", sizeof(current_student.learning_baseline) - 1);
		current_student.learning_baseline[sizeof(current_student.learning_baseline) - 1] = '\0';
	}

	if (goal) {
		strncpy(current_student.learning_goal, goal, sizeof(current_student.learning_goal) - 1);
		current_student.learning_goal[sizeof(current_student.learning_goal) - 1] = '\0';
		g_free(goal);
	} else {
		strncpy(current_student.learning_goal, "其他", sizeof(current_student.learning_goal) - 1);
		current_student.learning_goal[sizeof(current_student.learning_goal) - 1] = '\0';
	}

	save_data();

	GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
	                    GTK_DIALOG_MODAL,
	                    GTK_MESSAGE_INFO,
	                    GTK_BUTTONS_OK,
	                    "信息已保存！");
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);

	// 更新个人中心页面显示
	update_profile_display();
}

// 添加课程回调
void add_course_callback(GtkWidget *widget, gpointer data) {
	(void)data; // 忽略未使用参数
	GtkWidget *name_entry = g_object_get_data(G_OBJECT(widget), "name_entry");
	GtkWidget *time_entry = g_object_get_data(G_OBJECT(widget), "time_entry");
	GtkWidget *classroom_entry = g_object_get_data(G_OBJECT(widget), "classroom_entry");
	GtkWidget *day_combo = g_object_get_data(G_OBJECT(widget), "day_combo");

	// 检查控件是否有效
	if (!name_entry || !time_entry || !classroom_entry || !day_combo) {
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
		                    GTK_DIALOG_MODAL,
		                    GTK_MESSAGE_ERROR,
		                    GTK_BUTTONS_OK,
		                    "内部错误：控件绑定失败！");
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		return;
	}

	const char *course_name = gtk_entry_get_text(GTK_ENTRY(name_entry));

	// 输入验证
	if (course_name == NULL || strlen(course_name) == 0) {
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
		                    GTK_DIALOG_MODAL,
		                    GTK_MESSAGE_ERROR,
		                    GTK_BUTTONS_OK,
		                    "课程名称不能为空！");
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		return;
	}

	// 边界检查
	if (course_count >= MAX_COURSES) {
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
		                    GTK_DIALOG_MODAL,
		                    GTK_MESSAGE_ERROR,
		                    GTK_BUTTONS_OK,
		                    "课程数量已达上限！");
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		return;
	}

	strncpy(courses[course_count].course_name, course_name, sizeof(courses[course_count].course_name) - 1);
	courses[course_count].course_name[sizeof(courses[course_count].course_name) - 1] = '\0';

	strncpy(courses[course_count].time, gtk_entry_get_text(GTK_ENTRY(time_entry)), sizeof(courses[course_count].time) - 1);
	courses[course_count].time[sizeof(courses[course_count].time) - 1] = '\0';

	strncpy(courses[course_count].classroom, gtk_entry_get_text(GTK_ENTRY(classroom_entry)),
	        sizeof(courses[course_count].classroom) - 1);
	courses[course_count].classroom[sizeof(courses[course_count].classroom) - 1] = '\0';

	// 检查星期选择
	int day_selected = gtk_combo_box_get_active(GTK_COMBO_BOX(day_combo));
	if (day_selected < 0 || day_selected >= 7) {
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
		                    GTK_DIALOG_MODAL,
		                    GTK_MESSAGE_ERROR,
		                    GTK_BUTTONS_OK,
		                    "请选择星期！");
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		return;
	}
	courses[course_count].day = day_selected;

	course_count++;
	save_data();

	GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
	                    GTK_DIALOG_MODAL,
	                    GTK_MESSAGE_INFO,
	                    GTK_BUTTONS_OK,
	                    "课程添加成功！");
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);

	refresh_course_page();
	request_error_subject_update();
}

// 任务打卡回调
void add_task_callback(GtkWidget *widget, gpointer data) {
	(void)data; // 忽略未使用参数
	GtkWidget *name_entry = g_object_get_data(G_OBJECT(widget), "name_entry");
	GtkWidget *deadline_entry = g_object_get_data(G_OBJECT(widget), "deadline_entry");
	GtkWidget *desc_entry = g_object_get_data(G_OBJECT(widget), "desc_entry");
	GtkWidget *duration_entry = g_object_get_data(G_OBJECT(widget), "duration_entry");

	const char *task_name = gtk_entry_get_text(GTK_ENTRY(name_entry));
	const char *deadline = gtk_entry_get_text(GTK_ENTRY(deadline_entry));

	// 输入验证
	if (task_name == NULL || strlen(task_name) == 0) {
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
		                    GTK_DIALOG_MODAL,
		                    GTK_MESSAGE_ERROR,
		                    GTK_BUTTONS_OK,
		                    "任务名称不能为空！");
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		return;
	}

	// 边界检查
	if (task_count >= MAX_TASKS) {
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
		                    GTK_DIALOG_MODAL,
		                    GTK_MESSAGE_ERROR,
		                    GTK_BUTTONS_OK,
		                    "任务数量已达上限！");
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		return;
	}

	strncpy(tasks[task_count].task_name, task_name, sizeof(tasks[task_count].task_name) - 1);
	tasks[task_count].task_name[sizeof(tasks[task_count].task_name) - 1] = '\0';

	strncpy(tasks[task_count].deadline, deadline, sizeof(tasks[task_count].deadline) - 1);
	tasks[task_count].deadline[sizeof(tasks[task_count].deadline) - 1] = '\0';

	strncpy(tasks[task_count].description, gtk_entry_get_text(GTK_ENTRY(desc_entry)),
	        sizeof(tasks[task_count].description) - 1);
	tasks[task_count].description[sizeof(tasks[task_count].description) - 1] = '\0';

	// 获取时长
	const char *duration_str = gtk_entry_get_text(GTK_ENTRY(duration_entry));
	tasks[task_count].duration = duration_str && strlen(duration_str) > 0 ? atoi(duration_str) : 30;

	tasks[task_count].completed = 0;

	task_count++;
	save_data();

	GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
	                    GTK_DIALOG_MODAL,
	                    GTK_MESSAGE_INFO,
	                    GTK_BUTTONS_OK,
	                    "任务添加成功！");
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);

	update_task_store();
}

// 导入错题回调
void complete_task_callback(GtkWidget *widget, gpointer data) {
	(void)widget; // 忽略未使用参数
	int index = GPOINTER_TO_INT(data);

	// 边界检查
	if (index < 0 || index >= task_count) {
		g_print("Error: Invalid task index %d\n", index);
		return;
	}

	tasks[index].completed = 1;
	save_data();

	GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
	                    GTK_DIALOG_MODAL,
	                    GTK_MESSAGE_INFO,
	                    GTK_BUTTONS_OK,
	                    "任务已完成！");
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);

	update_task_store();
}

// Toggle按钮回调
void task_toggle_callback(GtkCellRendererToggle *renderer, gchar *path_str, gpointer data) {
	(void)renderer; // 忽略未使用参数
	GtkTreeModel *model = GTK_TREE_MODEL(data);
	GtkTreeIter iter;
	gboolean completed;
	gint row_index;

	if (gtk_tree_model_get_iter_from_string(model, &iter, path_str)) {
		gtk_tree_model_get(model, &iter, 3, &completed, -1);
		completed = !completed;

		// 获取任务行索引
		row_index = atoi(path_str);

		// 边界检查
		if (row_index >= 0 && row_index < task_count) {
			tasks[row_index].completed = completed;
			save_data();
		}

		gtk_list_store_set(GTK_LIST_STORE(model), &iter, 3, completed, -1);
	}
}

// 任务打卡回调
void set_mood_callback(GtkWidget *widget, gpointer data) {
	(void)data; // 忽略未使用参数
	GtkWidget *status_combo = g_object_get_data(G_OBJECT(widget), "status_combo");
	const char *statuses[] = {"高效", "突击", "放松", "迷茫", "疲惫"};

	// 边界检查
	if (mood_count >= MAX_MOOD) {
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
		                    GTK_DIALOG_MODAL,
		                    GTK_MESSAGE_ERROR,
		                    GTK_BUTTONS_OK,
		                    "心情记录已达上限");
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		return;
	}

	time_t now = time(0);
	struct tm *tm_now = localtime(&now);
	snprintf(mood_logs[mood_count].date, sizeof(mood_logs[mood_count].date), "%04d-%02d-%02d",
	         tm_now->tm_year + 1900, tm_now->tm_mon + 1, tm_now->tm_mday);

	int selected = gtk_combo_box_get_active(GTK_COMBO_BOX(status_combo));
	if (selected >= 0 && selected < 5) {
		strncpy(mood_logs[mood_count].status, statuses[selected], sizeof(mood_logs[mood_count].status) - 1);
		mood_logs[mood_count].status[sizeof(mood_logs[mood_count].status) - 1] = '\0';
		mood_count++;
		save_data();

		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
		                    GTK_DIALOG_MODAL,
		                    GTK_MESSAGE_INFO,
		                    GTK_BUTTONS_OK,
		                    "心情已记录成功");
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);

		update_mood_store();
		refresh_chart();
	}
}

// 根据心情推荐学习任务
void recommend_task_callback(GtkWidget *widget, gpointer data) {
	(void)data; // 忽略未使用参数

	// 获取今日心情
	const char *today_mood = "未记录";
	if (mood_count > 0) {
		today_mood = mood_logs[mood_count - 1].status;
	}

	// 根据心情生成推荐
	GString *recommendation = g_string_new(NULL);
	g_string_append(recommendation, "<b>根据您的心情推荐的学习任务：</b>\n\n");

	if (strcmp(today_mood, "高效") == 0) {
		g_string_append(recommendation, "<b>高效模式</b>\n");
		g_string_append(recommendation, "🎯 建议安排高难度的学习任务\n");
		g_string_append(recommendation, "📚 可以同时学习多门课程\n");
		g_string_append(recommendation, "⏰ 建议学习时长：2-3小时\n");
		g_string_append(recommendation, "💪 推荐任务类型：攻克难点、深度复习\n\n");
		g_string_append(recommendation, "推荐任务：\n");
		g_string_append(recommendation, "  1. 复习本周课程重难点\n");
		g_string_append(recommendation, "  2. 完成一门课程的专项练习\n");
		g_string_append(recommendation, "  3. 预习下周新课程\n");
	} else if (strcmp(today_mood, "突击") == 0) {
		g_string_append(recommendation, "⚡ <b>突击模式</b>\n");
		g_string_append(recommendation, "⚡ 建议安排紧迫的任务\n");
		g_string_append(recommendation, "🔥 集中精力完成一件事\n");
		g_string_append(recommendation, "⏰ 建议学习时长：1-2小时\n");
		g_string_append(recommendation, "📝 推荐任务类型：限时任务、作业完成\n\n");
		g_string_append(recommendation, "推荐任务：\n");
		g_string_append(recommendation, "  1. 完成即将到期的作业\n");
		g_string_append(recommendation, "  2. 准备明天的考试/测验\n");
		g_string_append(recommendation, "  3. 整理本周的学习笔记\n");
	} else if (strcmp(today_mood, "放松") == 0) {
		g_string_append(recommendation, "<b>放松模式</b>\n");
		g_string_append(recommendation, "😌 建议安排轻松的学习任务\n");
		g_string_append(recommendation, "🌟 可以学习感兴趣的内容\n");
		g_string_append(recommendation, "⏰ 建议学习时长：1小时左右\n");
		g_string_append(recommendation, "📖 推荐任务类型：阅读、兴趣探索\n\n");
		g_string_append(recommendation, "推荐任务：\n");
		g_string_append(recommendation, "  1. 阅读相关的课外书籍\n");
		g_string_append(recommendation, "  2. 观看学习视频教程\n");
		g_string_append(recommendation, "  3. 整理错题本\n");
	} else if (strcmp(today_mood, "迷茫") == 0) {
		g_string_append(recommendation, "<b>迷茫模式</b>\n");
		g_string_append(recommendation, "🧭 建议先梳理学习目标\n");
		g_string_append(recommendation, "💡 不要强迫自己学太难的内容\n");
		g_string_append(recommendation, "⏰ 建议学习时长：30分钟-1小时\n");
		g_string_append(recommendation, "📋 推荐任务类型：规划、回顾\n\n");
		g_string_append(recommendation, "推荐任务：\n");
		g_string_append(recommendation, "  1. 制定本周学习计划\n");
		g_string_append(recommendation, "  2. 回顾已学过的知识框架\n");
		g_string_append(recommendation, "  3. 列出学习中的疑问点\n");
	} else if (strcmp(today_mood, "疲惫") == 0) {
		g_string_append(recommendation, "<b>疲惫模式</b>\n");
		g_string_append(recommendation, "💤 建议适当休息\n");
		g_string_append(recommendation, "🍃 可以做轻松的学习任务\n");
		g_string_append(recommendation, "⏰ 建议学习时长：30分钟以内\n");
		g_string_append(recommendation, "🔄 推荐任务类型：简单复习、放松学习\n\n");
		g_string_append(recommendation, "推荐任务：\n");
		g_string_append(recommendation, "  1. 浏览今日课程的课件\n");
		g_string_append(recommendation, "  2. 快速浏览错题本\n");
		g_string_append(recommendation, "  3. 适当休息后再继续\n");
	} else {
		g_string_append(recommendation, "<b>未记录今日心情</b>\n");
		g_string_append(recommendation, "请先记录今日心情，我将为您推荐合适的学习任务。\n");
	}

	// 根据学习目标添加个性化建议
	if (strlen(current_student.learning_goal) > 0) {
		g_string_append(recommendation, "\n<b>基于您的学习目标（");
		g_string_append(recommendation, current_student.learning_goal);
		g_string_append(recommendation, "）的额外建议：</b>\n");

		if (strcmp(current_student.learning_goal, "考研") == 0) {
			g_string_append(recommendation, "📖 建议每天保持2小时以上的考研科目学习\n");
			g_string_append(recommendation, "🔢 重点复习数学和英语\n");

		} else if (strcmp(current_student.learning_goal, "就业") == 0) {
			g_string_append(recommendation, "💼 建议学习专业技能和实践项目\n");
			g_string_append(recommendation, "📄 可以开始准备简历和面试\n");

		} else if (strcmp(current_student.learning_goal, "出国") == 0) {
			g_string_append(recommendation, "🗣️ 建议每天学习外语\n");
			g_string_append(recommendation, "✈️ 关注留学相关的准备事项\n");

		} else if (strcmp(current_student.learning_goal, "考公") == 0) {
			g_string_append(recommendation, "📝 建议每天练习行测和申论\n");
			g_string_append(recommendation, "📰 关注时事政治和热点新闻\n");
		}
	}

	// 显示推荐对话框
	GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
	                    GTK_DIALOG_MODAL,
	                    GTK_MESSAGE_INFO,
	                    GTK_BUTTONS_OK,
	                    "%s", recommendation->str);
	gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG(dialog), recommendation->str);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);

	g_string_free(recommendation, TRUE);
}

// 更新系统时间显示
gboolean update_time(gpointer data) {
	(void)data;
	time_t now = time(NULL);
	struct tm *local_time = localtime(&now);

	char time_str[50];
	strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", local_time);

	gtk_label_set_text(GTK_LABEL(time_label), time_str);

	return G_SOURCE_CONTINUE;
}

// 获取数据文件路径（用户AppData目录）
char *get_data_file_path(const char *filename) {
#ifdef _WIN32
	static char data_path[512] = {0};
	if (data_path[0] == '\0') {
		// 使用用户AppData目录存储数据
		wchar_t appdata_path[512];
		SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, appdata_path);
		WideCharToMultiByte(CP_UTF8, 0, appdata_path, -1, data_path, sizeof(data_path), NULL, NULL);
		// 创建学习管理系统子目录
		strncat(data_path, "\\学习管理系统\\", sizeof(data_path) - strlen(data_path) - 1);
	}
#else
	static char data_path[512] = {0};
	if (data_path[0] == '\0') {
		ssize_t len = readlink("/proc/self/exe", data_path, sizeof(data_path) - 1);
		if (len != -1) {
			data_path[len] = '\0';
			char *last_slash = strrchr(data_path, '/');
			if (last_slash) *(last_slash + 1) = '\0';
		}
	}
#endif
	static char full_path[1024];
	snprintf(full_path, sizeof(full_path), "%s%s", data_path, filename);
	return full_path;
}

// 深色模式切换
void toggle_dark_mode(GtkWidget *widget, gpointer data) {
	(void)widget;
	(void)data;
	dark_mode = !dark_mode;

	GtkStyleContext *ctx = gtk_widget_get_style_context(main_window);
	if (dark_mode) {
		gtk_style_context_add_class(ctx, "dark");
		gtk_button_set_label(GTK_BUTTON(widget), "浅色");
	} else {
		gtk_style_context_remove_class(ctx, "dark");
		gtk_button_set_label(GTK_BUTTON(widget), "深色");
	}
	gtk_widget_queue_draw(main_window);
	refresh_chart();
}

// 导入错题回调
void import_error_question_callback(GtkWidget *widget, gpointer data) {
	(void)data; // 忽略未使用参数

	// 创建文件选择对话框
	GtkWidget *dialog = gtk_file_chooser_dialog_new("选择题目图片",
	                    GTK_WINDOW(main_window),
	                    GTK_FILE_CHOOSER_ACTION_OPEN,
	                    "_取消", GTK_RESPONSE_CANCEL,
	                    "_确定", GTK_RESPONSE_ACCEPT,
	                    NULL);

	// 添加图片文件过滤器
	GtkFileFilter *filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "图片文件");
	gtk_file_filter_add_pattern(filter, "*.png");
	gtk_file_filter_add_pattern(filter, "*.jpg");
	gtk_file_filter_add_pattern(filter, "*.jpeg");
	gtk_file_filter_add_pattern(filter, "*.bmp");
	gtk_file_filter_add_pattern(filter, "*.gif");
	gtk_file_filter_add_pattern(filter, "*.PNG");
	gtk_file_filter_add_pattern(filter, "*.JPG");
	gtk_file_filter_add_pattern(filter, "*.JPEG");
	gtk_file_filter_add_pattern(filter, "*.BMP");
	gtk_file_filter_add_pattern(filter, "*.GIF");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

	gint response = gtk_dialog_run(GTK_DIALOG(dialog));

	if (response == GTK_RESPONSE_ACCEPT) {
		char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

		// 检查filename是否为NULL

		if (!filename) {
			gtk_widget_destroy(dialog);
			return;
		}

		// 使用GIO检查文件是否存在且可读

		if (!g_file_test(filename, G_FILE_TEST_EXISTS) || !g_file_test(filename, G_FILE_TEST_IS_REGULAR)) {
			GtkWidget *error_dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
			                          GTK_DIALOG_MODAL,
			                          GTK_MESSAGE_ERROR,
			                          GTK_BUTTONS_OK,
			                          "图片文件不存在或无法访问！");
			gtk_dialog_run(GTK_DIALOG(error_dialog));
			gtk_widget_destroy(error_dialog);
			g_free(filename);
			gtk_widget_destroy(dialog);
			return;
		}

		// 检查文件大小（限制在10MB以内）
		GFile *gfile = g_file_new_for_path(filename);
		GFileInfo *info = g_file_query_info(gfile, G_FILE_ATTRIBUTE_STANDARD_SIZE, G_FILE_QUERY_INFO_NONE, NULL, NULL);
		goffset file_size = g_file_info_get_size(info);
		g_object_unref(info);
		g_object_unref(gfile);

		if (file_size > 10 * 1024 * 1024) {
			GtkWidget *error_dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
			                          GTK_DIALOG_MODAL,
			                          GTK_MESSAGE_ERROR,
			                          GTK_BUTTONS_OK,
			                          "图片文件过大，请选择小于10MB的图片！");
			gtk_dialog_run(GTK_DIALOG(error_dialog));
			gtk_widget_destroy(error_dialog);
			g_free(filename);
			gtk_widget_destroy(dialog);
			return;
		}

		// 边界检查 - 确保错题数量未达上限

		if (error_count >= MAX_ERRORS) {
			GtkWidget *err_dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
			                        GTK_DIALOG_MODAL,
			                        GTK_MESSAGE_ERROR,
			                        GTK_BUTTONS_OK,
			                        "错题数量已达上限！");
			gtk_dialog_run(GTK_DIALOG(err_dialog));
			gtk_widget_destroy(err_dialog);
			g_free(filename);
			gtk_widget_destroy(dialog);
			return;
		}

		// 复制文件路径（使用g_strdup确保安全）
		g_strlcpy(error_questions[error_count].question_path, filename, sizeof(error_questions[error_count].question_path));

		// 获取科目信息（从课程表选择）
		GtkWidget *subject_combo = g_object_get_data(G_OBJECT(widget), "subject_combo");

		if (!subject_combo) {
			strncpy(error_questions[error_count].subject, "未分类", sizeof(error_questions[error_count].subject) - 1);


		} else {
			gchar *selected_subject = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(subject_combo));

			if (selected_subject && strlen(selected_subject) > 0) {
				// 检查是否有"暂无课程"的提示文本
				if (strstr(selected_subject, "暂无课程") == NULL) {
					g_strlcpy(error_questions[error_count].subject, selected_subject, sizeof(error_questions[error_count].subject));
				} else {
					g_strlcpy(error_questions[error_count].subject, "未分类", sizeof(error_questions[error_count].subject));
				}
			} else {
				g_strlcpy(error_questions[error_count].subject, "未分类", sizeof(error_questions[error_count].subject));
			}
			g_free(selected_subject);
		}

		// 获取难度信息
		GtkWidget *diff_combo = g_object_get_data(G_OBJECT(widget), "diff_combo");

		const char *difficulties[] = {"难", "中等", "易", "基础"};
		int selected = gtk_combo_box_get_active(GTK_COMBO_BOX(diff_combo));

		if (selected >= 0 && selected < 4) {
			g_strlcpy(error_questions[error_count].difficulty, difficulties[selected],
			          sizeof(error_questions[error_count].difficulty));


		} else {
			g_strlcpy(error_questions[error_count].difficulty, "中等",
			          sizeof(error_questions[error_count].difficulty));
		}

		error_count++;
		save_data();

		GtkWidget *info_dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
		                         GTK_DIALOG_MODAL,
		                         GTK_MESSAGE_INFO,
		                         GTK_BUTTONS_OK,
		                         "错题已添加成功！");
		gtk_dialog_run(GTK_DIALOG(info_dialog));
		gtk_widget_destroy(info_dialog);

		g_free(filename);
		update_error_store();
	}

	gtk_widget_destroy(dialog);
}

// 绘制图表回调 - 统计今日任务完成时间占比
gboolean draw_chart_callback(GtkWidget *widget, cairo_t *cr, gpointer data) {
	(void)data; // 忽略未使用参数
	GtkAllocation allocation;
	gtk_widget_get_allocation(widget, &allocation);

	int width = allocation.width;
	int height = allocation.height;
	int center_x = width / 2;
	int center_y = height / 2;
	int radius = (width < height ? width : height) / 3;

	// 计算已完成任务的总时间和各任务时间占比
	int total_duration = 0;
	int completed_count = 0;
	for (int i = 0; i < task_count; i++) {
		if (tasks[i].completed) {
			total_duration += tasks[i].duration;
			completed_count++;
		}
	}

	// 定义颜色数组（暗色模式下使用更亮的颜色）
	double colors[6][3];
	if (dark_mode) {
		double dc[6][3] = {
			{0.3, 0.7, 0.9},  // 亮蓝
			{0.3, 0.85, 0.3}, // 亮绿
			{0.95, 0.7, 0.3}, // 亮橙
			{0.85, 0.4, 0.85},// 亮紫
			{0.9, 0.4, 0.4},  // 亮红
			{0.85, 0.85, 0.3} // 亮黄
		};
		memcpy(colors, dc, sizeof(dc));
	} else {
		double lc[6][3] = {
			{0.2, 0.6, 0.8},
			{0.2, 0.7, 0.2},
			{0.8, 0.6, 0.2},
			{0.7, 0.3, 0.7},
			{0.8, 0.3, 0.3},
			{0.6, 0.6, 0.2}
		};
		memcpy(colors, lc, sizeof(lc));
	}

	// 绘制饼图
	double start_angle = 0;
	for (int i = 0; i < task_count; i++) {
		if (tasks[i].completed && tasks[i].duration > 0 && total_duration > 0) {
			double ratio = (double)tasks[i].duration / total_duration;
			double end_angle = start_angle + 2 * M_PI * ratio;

			// 设置颜色
			int color_idx = i % 6;
			cairo_set_source_rgb(cr, colors[color_idx][0], colors[color_idx][1], colors[color_idx][2]);

			// 绘制扇形
			cairo_move_to(cr, center_x, center_y);
			cairo_arc(cr, center_x, center_y, radius, start_angle, end_angle);
			cairo_close_path(cr);
			cairo_fill(cr);

			start_angle = end_angle;
		}
	}

	// 绘制边框
	if (dark_mode)
		cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
	else
		cairo_set_source_rgb(cr, 0.3, 0.3, 0.3);
	cairo_set_line_width(cr, 2);
	cairo_arc(cr, center_x, center_y, radius, 0, 2 * M_PI);
	cairo_stroke(cr);

	// 绘制中心文字 - 使用支持中文的字体
	cairo_select_font_face(cr, "SimHei", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_font_size(cr, 14);
	if (dark_mode)
		cairo_set_source_rgb(cr, 0.93, 0.93, 0.93);
	else
		cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);

	char text[100];
	snprintf(text, sizeof(text), "已完成: %d个任务", completed_count);
	cairo_move_to(cr, center_x - 60, center_y - 20);
	cairo_show_text(cr, text);

	snprintf(text, sizeof(text), "总耗时: %d分钟", total_duration);
	cairo_move_to(cr, center_x - 60, center_y + 10);
	cairo_show_text(cr, text);

	// 绘制图例
	cairo_set_font_size(cr, 11);
	int legend_y = 20;
	int color_idx = 0;
	for (int i = 0; i < task_count && legend_y < height - 20; i++) {
		if (tasks[i].completed && tasks[i].duration > 0) {
			cairo_set_source_rgb(cr, colors[color_idx][0], colors[color_idx][1], colors[color_idx][2]);
			cairo_rectangle(cr, 10, legend_y, 15, 10);
			cairo_fill(cr);

			cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
			if (dark_mode)
				cairo_set_source_rgb(cr, 0.88, 0.88, 0.88);
			snprintf(text, sizeof(text), "%s (%dmin)", tasks[i].task_name, tasks[i].duration);
			cairo_move_to(cr, 35, legend_y + 9);
			cairo_show_text(cr, text);

			legend_y += 20;
			color_idx++;
		}
	}

	if (completed_count == 0) {
		cairo_set_font_size(cr, 14);
		if (dark_mode)
			cairo_set_source_rgb(cr, 0.6, 0.6, 0.6);
		snprintf(text, sizeof(text), "暂无已完成任务");
		cairo_move_to(cr, center_x - 60, center_y);
		cairo_show_text(cr, text);
	}

	return TRUE;
}

// 创建主窗口
void create_main_window(GtkApplication *app, gpointer user_data) {
	(void)user_data;

	// ========== 加载全局 CSS 样式表（令牌化 + 深色模式） ==========
	GtkCssProvider *css_provider = gtk_css_provider_new();
	const char *css_data =
		"/* ===== 颜色令牌 ===== */\n"
		"@define-color bg-primary #f0f2f5;\n"
		"@define-color bg-card #ffffff;\n"
		"@define-color bg-nav-from #667eea;\n"
		"@define-color bg-nav-to #764ba2;\n"
		"@define-color text-primary #1f2937;\n"
		"@define-color text-secondary #6b7280;\n"
		"@define-color text-muted #9ca3af;\n"
		"@define-color border-light #e5e7eb;\n"
		"@define-color border-input #d1d5db;\n"
		"@define-color accent-blue #3b82f6;\n"
		"@define-color accent-green #10b981;\n"
		"@define-color accent-orange #f97316;\n"
		"@define-color accent-purple #8b5cf6;\n"
		"@define-color accent-red #ef4444;\n"
		"@define-color accent-cyan #06b6d4;\n"
		"@define-color accent-indigo #6366f1;\n"
		"@define-color row-even #f9fafb;\n"
		"@define-color row-hover #e0e7ff;\n"
		"@define-color row-selected #c7d2fe;\n"
		"@define-color shadow-color rgba(0,0,0,0.06);\n"
		"\n"
		"/* ===== 全局基础 ===== */\n"
		"* { font-family: 'Microsoft YaHei', 'SimHei', 'Noto Sans CJK SC', sans-serif; }\n"
		"\n"
		"window, .window-bg { background-color: @bg-primary; }\n"
		"\n"
		"/* ===== 导航栏 ===== */\n"
		".nav-bar { background: linear-gradient(135deg, @bg-nav-from 0%, @bg-nav-to 100%); padding: 8px 12px; }\n"
		".nav-button {\n"
		"    background: rgba(255,255,255,0.15); color: white; border: 1px solid rgba(255,255,255,0.25);\n"
		"    border-radius: 6px; padding: 6px 14px; font-size: 13px; font-weight: bold;\n"
		"    transition: all 0.25s cubic-bezier(0.4, 0, 0.2, 1); margin: 0 2px;\n"
		"}\n"
		".nav-button:hover { background: rgba(255,255,255,0.30); border-color: rgba(255,255,255,0.55); box-shadow: 0 2px 6px rgba(0,0,0,0.12); }\n"
		".nav-button:active { background: rgba(255,255,255,0.38); }\n"
		".nav-label { color: white; font-weight: bold; font-size: 13px; }\n"
		".nav-theme-btn { background: rgba(255,255,255,0.12); color: white; border: 1px solid rgba(255,255,255,0.20); border-radius: 20px; padding: 5px 10px; font-size: 12px; font-weight: bold; transition: all 0.25s cubic-bezier(0.4, 0, 0.2, 1); margin-left: 8px; }\n"
		".nav-theme-btn:hover { background: rgba(255,255,255,0.28); border-color: rgba(255,255,255,0.45); }\n"
		"\n"
		".page-container { background-color: @bg-primary; }\n"
		"\n"
		".page-title { font-size: 22px; font-weight: bold; padding: 8px 0; }\n"
		".page-title-student { color: @accent-blue; }\n"
		".page-title-course { color: @accent-green; }\n"
		".page-title-task { color: @accent-orange; }\n"
		".page-title-mood { color: @accent-purple; }\n"
		".page-title-chart { color: @accent-cyan; }\n"
		".page-title-error { color: @accent-red; }\n"
		".page-title-profile { color: @accent-indigo; }\n"
		"\n"
		".card-frame {\n"
		"    background: @bg-card; border-radius: 10px; border: none;\n"
		"    box-shadow: 0 2px 8px @shadow-color; margin: 6px 0; padding: 0;\n"
		"}\n"
		".card-frame > label {\n"
		"    font-weight: bold; font-size: 15px; color: @text-primary;\n"
		"    padding: 12px 16px 8px 16px;\n"
		"}\n"
		".card-frame-student > label { color: @accent-blue; }\n"
		".card-frame-course > label { color: @accent-green; }\n"
		".card-frame-task > label { color: @accent-orange; }\n"
		".card-frame-mood > label { color: @accent-purple; }\n"
		".card-frame-chart > label { color: @accent-cyan; }\n"
		".card-frame-error > label { color: @accent-red; }\n"
		".card-frame-profile > label { color: @accent-indigo; }\n"
		"\n"
		".course-card {\n"
		"    background: linear-gradient(135deg, #ecfdf5 0%, #d1fae5 100%);\n"
		"    border: 1px solid #a7f3d0; border-radius: 8px;\n"
		"    box-shadow: 0 1px 4px rgba(0,0,0,0.04);\n"
		"    padding: 4px; margin: 2px;\n"
		"}\n"
		".course-card-name { font-weight: bold; font-size: 13px; color: #065f46; }\n"
		".course-card-time { font-size: 12px; color: #047857; }\n"
		".course-card-room { font-size: 11px; color: @text-secondary; }\n"
		"\n"
		".form-label { font-size: 13px; font-weight: bold; color: @text-primary; }\n"
		"\n"
		"button { border-radius: 6px; padding: 6px 16px; font-weight: bold; transition: all 0.25s cubic-bezier(0.4, 0, 0.2, 1); }\n"
		".btn-student { color: white; background: linear-gradient(135deg, @accent-blue, #2563eb); border: none; }\n"
		".btn-student:hover { background: linear-gradient(135deg, #2563eb, #1d4ed8); box-shadow: 0 2px 10px rgba(59,130,246,0.4); }\n"
		".btn-course { color: white; background: linear-gradient(135deg, @accent-green, #059669); border: none; }\n"
		".btn-course:hover { background: linear-gradient(135deg, #059669, #047857); box-shadow: 0 2px 10px rgba(16,185,129,0.4); }\n"
		".btn-task { color: white; background: linear-gradient(135deg, @accent-orange, #ea580c); border: none; }\n"
		".btn-task:hover { background: linear-gradient(135deg, #ea580c, #c2410c); box-shadow: 0 2px 10px rgba(249,115,22,0.4); }\n"
		".btn-mood { color: white; background: linear-gradient(135deg, @accent-purple, #7c3aed); border: none; }\n"
		".btn-mood:hover { background: linear-gradient(135deg, #7c3aed, #6d28d9); box-shadow: 0 2px 10px rgba(139,92,246,0.4); }\n"
		".btn-error { color: white; background: linear-gradient(135deg, @accent-red, #dc2626); border: none; }\n"
		".btn-error:hover { background: linear-gradient(135deg, #dc2626, #b91c1c); box-shadow: 0 2px 10px rgba(239,68,68,0.4); }\n"
		".btn-chart { color: white; background: linear-gradient(135deg, @accent-cyan, #0891b2); border: none; }\n"
		".btn-chart:hover { background: linear-gradient(135deg, #0891b2, #0e7490); box-shadow: 0 2px 10px rgba(6,182,212,0.4); }\n"
		"\n"
		"entry { border-radius: 6px; border: 1px solid @border-input; padding: 6px 10px; font-size: 13px; }\n"
		"entry:focus { border-color: @bg-nav-from; box-shadow: 0 0 0 3px rgba(102,126,234,0.15); }\n"
		"\n"
		"treeview { border-radius: 8px; background: @bg-card; }\n"
		"treeview header button { background: linear-gradient(135deg, @row-even, #f3f4f6); color: @text-primary; font-weight: bold; padding: 8px; border-bottom: 2px solid @border-light; }\n"
		"treeview row:nth-child(even) { background: @row-even; }\n"
		"treeview row:hover { background: @row-hover; }\n"
		"treeview row:selected { background: @row-selected; color: #1e3a5f; }\n"
		"\n"
		"scrolledwindow { border-radius: 8px; border: 1px solid @border-light; }\n"
		"\n"
		".stats-bar { background: @bg-card; border-radius: 8px; padding: 10px 14px; box-shadow: 0 1px 4px @shadow-color; margin: 4px 0; }\n"
		".time-label { font-size: 12px; color: @text-muted; }\n"
		"\n"
		"menubar { background: @bg-card; border-bottom: 1px solid @border-light; padding: 2px 0; }\n"
		"\n"
		".tip-label { font-size: 11px; color: @text-muted; padding: 4px 0; }\n"
		"\n"
		"/* ===== 深色模式 ===== */\n"
		".dark, .dark window, .dark .window-bg { background-color: #1a1a2e; }\n"
		".dark .page-container { background-color: #1a1a2e; }\n"
		".dark .card-frame { background: #252540; box-shadow: 0 2px 8px rgba(0,0,0,0.25); }\n"
		".dark .card-frame > label { color: #e2e8f0; }\n"
		".dark .card-frame-student > label { color: #60a5fa; }\n"
		".dark .card-frame-course > label { color: #34d399; }\n"
		".dark .card-frame-task > label { color: #fb923c; }\n"
		".dark .card-frame-mood > label { color: #a78bfa; }\n"
		".dark .card-frame-chart > label { color: #22d3ee; }\n"
		".dark .card-frame-error > label { color: #f87171; }\n"
		".dark .card-frame-profile > label { color: #818cf8; }\n"
		".dark .page-title-student { color: #60a5fa; }\n"
		".dark .page-title-course { color: #34d399; }\n"
		".dark .page-title-task { color: #fb923c; }\n"
		".dark .page-title-mood { color: #a78bfa; }\n"
		".dark .page-title-chart { color: #22d3ee; }\n"
		".dark .page-title-error { color: #f87171; }\n"
		".dark .page-title-profile { color: #818cf8; }\n"
		".dark .form-label { color: #e2e8f0; }\n"
		".dark entry { background: #2d2d4a; color: #e2e8f0; border-color: #4a4a6a; }\n"
		".dark entry:focus { border-color: #818cf8; }\n"
		".dark treeview { background: #252540; color: #e2e8f0; }\n"
		".dark treeview header button { background: #2d2d4a; color: #e2e8f0; border-bottom: 2px solid #4a4a6a; }\n"
		".dark treeview row:nth-child(even) { background: #2a2a45; }\n"
		".dark treeview row:hover { background: #3b3b60; }\n"
		".dark treeview row:selected { background: #4c4c78; color: #e2e8f0; }\n"
		".dark scrolledwindow { border-color: #4a4a6a; }\n"
		".dark .stats-bar { background: #252540; box-shadow: 0 1px 4px rgba(0,0,0,0.25); }\n"
		".dark .stats-bar label, .dark .stats-bar * { color: #e2e8f0; }\n"
		".dark .time-label { color: #6b7280; }\n"
		".dark menubar { background: #252540; border-bottom: 1px solid #4a4a6a; }\n"
		".dark menubar > menuitem > label { color: #e2e8f0; }\n"
		".dark .tip-label { color: #6b7280; }\n"
		".dark .course-card { background: linear-gradient(135deg, #1e3a3a 0%, #0f2f2f 100%); border-color: #2d5a3d; }\n"
		".dark .course-card-name { color: #6ee7b7; }\n"
		".dark .course-card-time { color: #34d399; }\n"
		".dark .course-card-room { color: #9ca3af; }\n"
		".dark .nav-bar { background: linear-gradient(135deg, #4c1d95 0%, #5b21b6 100%); }\n"
		".dark combo box, .dark combobox { background: #2d2d4a; color: #e2e8f0; }\n"
		".dark button { transition: all 0.25s cubic-bezier(0.4, 0, 0.2, 1); }\n"
		".dark label { color: #e2e8f0; }\n";

	gtk_css_provider_load_from_data(css_provider, css_data, -1, NULL);
	gtk_style_context_add_provider_for_screen(
		gdk_screen_get_default(),
		GTK_STYLE_PROVIDER(css_provider),
		GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
	);
	g_object_unref(css_provider);
	// ========== CSS 加载完毕 ==========

	main_window = gtk_application_window_new(app);
	gtk_window_set_title(GTK_WINDOW(main_window), "学习管理系统");
	gtk_window_set_default_size(GTK_WINDOW(main_window), 1000, 680);
	gtk_window_set_position(GTK_WINDOW(main_window), GTK_WIN_POS_CENTER);

	// 设置UTF-8编码支持
#ifdef _WIN32
	// Windows 下使用 setlocale
	setlocale(LC_ALL, ".UTF-8");
#endif

	// 主容器 - 使用GtkGrid便于响应式布局和右下角时间标签
	GtkWidget *main_grid = gtk_grid_new();
	gtk_container_add(GTK_CONTAINER(main_window), main_grid);

	// 主内容区域
	GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_hexpand(main_vbox, TRUE);
	gtk_widget_set_vexpand(main_vbox, TRUE);
	gtk_grid_attach(GTK_GRID(main_grid), main_vbox, 0, 0, 1, 1);

	// 系统时间显示标签（右下角）
	time_label = gtk_label_new("");
	gtk_widget_set_size_request(time_label, 180, -1);
	gtk_label_set_justify(GTK_LABEL(time_label), GTK_JUSTIFY_RIGHT);
	gtk_grid_attach(GTK_GRID(main_grid), time_label, 0, 1, 1, 1);
	gtk_widget_set_halign(GTK_WIDGET(main_grid), GTK_ALIGN_FILL);
	gtk_widget_set_valign(GTK_WIDGET(main_grid), GTK_ALIGN_FILL);
	gtk_widget_set_halign(time_label, GTK_ALIGN_END);
	gtk_widget_set_valign(time_label, GTK_ALIGN_END);

	// 更新时间显示
	update_time(NULL);
	// 设置定时器，每秒更新一次时间
	g_timeout_add_seconds(1, (GSourceFunc)update_time, NULL);

	// 菜单栏
	GtkWidget *menubar = gtk_menu_bar_new();
	GtkWidget *menu_item;
	GtkWidget *submenu;
	GtkWidget *menu;

	menu_item = gtk_menu_item_new_with_label("首页");
	submenu = gtk_menu_new();
	GtkWidget *home_item = gtk_menu_item_new_with_label("返回首页");
	g_signal_connect(home_item, "activate", G_CALLBACK(show_student_info_page), NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(submenu), home_item);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), submenu);
	gtk_menu_shell_append(GTK_MENU_SHELL(menubar), menu_item);

	// 导航按钮
	GtkWidget *nav_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3);
	{
		GtkStyleContext *ctx_nav = gtk_widget_get_style_context(nav_hbox);
		gtk_style_context_add_class(ctx_nav, "nav-bar");
	}
	GtkWidget *nav_label = gtk_label_new("导航");
	{
		GtkStyleContext *ctx_nl = gtk_widget_get_style_context(nav_label);
		gtk_style_context_add_class(ctx_nl, "nav-label");
	}
	gtk_box_pack_start(GTK_BOX(nav_hbox), nav_label, FALSE, FALSE, 8);

	GtkWidget *btn_student = gtk_button_new_with_label("学生信息");
	GtkWidget *btn_course = gtk_button_new_with_label("课程表");
	GtkWidget *btn_task = gtk_button_new_with_label("学习任务");
	GtkWidget *btn_mood = gtk_button_new_with_label("心情记录");
	GtkWidget *btn_chart = gtk_button_new_with_label("学习统计");
	GtkWidget *btn_error = gtk_button_new_with_label("错题集");
	GtkWidget *btn_profile = gtk_button_new_with_label("个人中心");
	GtkWidget *btn_help = gtk_button_new_with_label("使用说明");

	// 给所有导航按钮添加 CSS class
	{
		GtkWidget *nav_btns[] = {btn_student, btn_course, btn_task, btn_mood, btn_chart, btn_error, btn_profile, btn_help};
		int nav_btn_count = sizeof(nav_btns) / sizeof(nav_btns[0]);
		for (int bi = 0; bi < nav_btn_count; bi++) {
			GtkStyleContext *ctx = gtk_widget_get_style_context(nav_btns[bi]);
			gtk_style_context_add_class(ctx, "nav-button");
		}
	}

	g_signal_connect(btn_student, "clicked", G_CALLBACK(show_student_info_page), NULL);
	g_signal_connect(btn_course, "clicked", G_CALLBACK(show_course_page), NULL);
	g_signal_connect(btn_task, "clicked", G_CALLBACK(show_task_page), NULL);
	g_signal_connect(btn_mood, "clicked", G_CALLBACK(show_mood_page), NULL);
	g_signal_connect(btn_chart, "clicked", G_CALLBACK(show_chart_page), NULL);
	g_signal_connect(btn_error, "clicked", G_CALLBACK(show_error_page), NULL);
	g_signal_connect(btn_profile, "clicked", G_CALLBACK(show_profile_page), NULL);
	g_signal_connect(btn_help, "clicked", G_CALLBACK(show_help_callback), NULL);

	gtk_box_pack_start(GTK_BOX(nav_hbox), btn_student, FALSE, FALSE, 1);
	gtk_box_pack_start(GTK_BOX(nav_hbox), btn_course, FALSE, FALSE, 1);
	gtk_box_pack_start(GTK_BOX(nav_hbox), btn_task, FALSE, FALSE, 1);
	gtk_box_pack_start(GTK_BOX(nav_hbox), btn_mood, FALSE, FALSE, 1);
	gtk_box_pack_start(GTK_BOX(nav_hbox), btn_chart, FALSE, FALSE, 1);
	gtk_box_pack_start(GTK_BOX(nav_hbox), btn_error, FALSE, FALSE, 1);
	gtk_box_pack_start(GTK_BOX(nav_hbox), btn_profile, FALSE, FALSE, 1);
	gtk_box_pack_start(GTK_BOX(nav_hbox), btn_help, FALSE, FALSE, 1);

	// 深色模式切换按钮
	GtkWidget *btn_theme = gtk_button_new_with_label("深色");
	{
		GtkStyleContext *ctx_bt = gtk_widget_get_style_context(btn_theme);
		gtk_style_context_add_class(ctx_bt, "nav-theme-btn");
	}
	g_signal_connect(btn_theme, "clicked", G_CALLBACK(toggle_dark_mode), NULL);
	gtk_box_pack_end(GTK_BOX(nav_hbox), btn_theme, FALSE, FALSE, 1);

	gtk_box_pack_start(GTK_BOX(main_vbox), menubar, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(main_vbox), nav_hbox, FALSE, FALSE, 5);

	// 页面栈 - 撑满剩余空间
	stack = gtk_stack_new();
	gtk_widget_set_hexpand(stack, TRUE);
	gtk_widget_set_vexpand(stack, TRUE);
	gtk_box_pack_end(GTK_BOX(main_vbox), stack, TRUE, TRUE, 0);

	// 初始化数据(先加载再创建页面)
	load_data();

	// 创建页面
	show_student_info_page();
	show_course_page();
	show_task_page();
	show_mood_page();
	show_chart_page();
	show_error_page();
	show_profile_page();

	// 默认显示第一个页面
	gtk_stack_set_visible_child(GTK_STACK(stack), student_info_page);

	// 连接关闭事件处理函数，确保关闭时保存数据
	g_signal_connect(main_window, "delete-event", G_CALLBACK(on_main_window_delete), NULL);

	// 显示主窗口
	gtk_widget_show_all(main_window);
}

// 关闭窗口时保存数据
gboolean on_main_window_delete(GtkWidget *widget, GdkEvent *event, gpointer user_data) {
	(void)event;
	(void)user_data;
	
	save_data();
	
	return FALSE; // 返回 FALSE 允许窗口关闭
}

// 显示学生信息页面
void show_student_info_page() {
	// 如果页面已存在，只需切换显示
	if (student_info_page != NULL) {
		// 更新学生信息显示
		update_student_info_display();
		gtk_stack_set_visible_child(GTK_STACK(stack), student_info_page);
		return;
	}

	student_info_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
	gtk_widget_set_margin_start(student_info_page, 24);
	gtk_widget_set_margin_end(student_info_page, 24);
	gtk_widget_set_margin_top(student_info_page, 16);
	gtk_widget_set_margin_bottom(student_info_page, 16);

	GtkWidget *title = gtk_label_new("学生信息");
	{
		GtkStyleContext *ctx_t = gtk_widget_get_style_context(title);
		gtk_style_context_add_class(ctx_t, "page-title");
		gtk_style_context_add_class(ctx_t, "page-title-student");
	}
	gtk_box_pack_start(GTK_BOX(student_info_page), title, FALSE, FALSE, 8);

	// 学生信息卡片
	GtkWidget *student_form_frame = gtk_frame_new(NULL);
	{
		GtkStyleContext *ctx_sff = gtk_widget_get_style_context(student_form_frame);
		gtk_style_context_add_class(ctx_sff, "card-frame");
		gtk_style_context_add_class(ctx_sff, "card-frame-student");
	}
	gtk_box_pack_start(GTK_BOX(student_info_page), student_form_frame, TRUE, TRUE, 6);

	GtkWidget *grid = gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(grid), 12);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
	gtk_widget_set_margin_start(grid, 16);
	gtk_widget_set_margin_end(grid, 16);
	gtk_widget_set_margin_top(grid, 12);
	gtk_widget_set_margin_bottom(grid, 12);
	gtk_container_add(GTK_CONTAINER(student_form_frame), grid);

	// 姓名
	GtkWidget *name_label = gtk_label_new("姓名:");
	gtk_style_context_add_class(gtk_widget_get_style_context(name_label), "form-label");
	gtk_grid_attach(GTK_GRID(grid), name_label, 0, 0, 1, 1);
	GtkWidget *name_entry = gtk_entry_new();
	student_name_entry = name_entry;
	gtk_entry_set_text(GTK_ENTRY(name_entry), current_student.name);
	gtk_grid_attach(GTK_GRID(grid), name_entry, 1, 0, 1, 1);

	// 专业
		GtkWidget *major_label = gtk_label_new("专业:");
	gtk_style_context_add_class(gtk_widget_get_style_context(major_label), "form-label");
	gtk_grid_attach(GTK_GRID(grid), major_label, 0, 1, 1, 1);
	GtkWidget *major_entry = gtk_entry_new();
	student_major_entry = major_entry;
	gtk_entry_set_text(GTK_ENTRY(major_entry), current_student.major);
	gtk_grid_attach(GTK_GRID(grid), major_entry, 1, 1, 1, 1);

	// 年级
		GtkWidget *grade_label = gtk_label_new("年级:");
	gtk_style_context_add_class(gtk_widget_get_style_context(grade_label), "form-label");
	gtk_grid_attach(GTK_GRID(grid), grade_label, 0, 2, 1, 1);
	GtkWidget *grade_entry = gtk_entry_new();
	student_grade_entry = grade_entry;
	char grade_str[10];
	snprintf(grade_str, sizeof(grade_str), "%d", current_student.grade);
	gtk_entry_set_text(GTK_ENTRY(grade_entry), grade_str);
	gtk_grid_attach(GTK_GRID(grid), grade_entry, 1, 2, 1, 1);

	// 学习基础
		GtkWidget *baseline_label = gtk_label_new("学习基础:");
	gtk_style_context_add_class(gtk_widget_get_style_context(baseline_label), "form-label");
	gtk_grid_attach(GTK_GRID(grid), baseline_label, 0, 3, 1, 1);
	GtkWidget *baseline_combo = gtk_combo_box_text_new();
	student_baseline_combo = baseline_combo;
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(baseline_combo), "薄弱");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(baseline_combo), "一般");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(baseline_combo), "良好");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(baseline_combo), "优秀");
	// 设置当前值
	const char *baseline = current_student.learning_baseline;
	if (strlen(baseline) > 0) {
		for (int i = 0; i < 4; i++) {
			const char *items[] = {"薄弱", "一般", "良好", "优秀"};
			if (strcmp(baseline, items[i]) == 0) {
				gtk_combo_box_set_active(GTK_COMBO_BOX(baseline_combo), i);
				break;
			}
		}
	} else {
		gtk_combo_box_set_active(GTK_COMBO_BOX(baseline_combo), 0);
	}
	gtk_grid_attach(GTK_GRID(grid), baseline_combo, 1, 3, 1, 1);

	// 学习目标
		GtkWidget *goal_label = gtk_label_new("学习目标:");
	gtk_style_context_add_class(gtk_widget_get_style_context(goal_label), "form-label");
	gtk_grid_attach(GTK_GRID(grid), goal_label, 0, 4, 1, 1);
	GtkWidget *goal_combo = gtk_combo_box_text_new();
	student_goal_combo = goal_combo;
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(goal_combo), "考研");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(goal_combo), "就业");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(goal_combo), "出国");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(goal_combo), "考公");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(goal_combo), "其他");
	// 设置当前值
	const char *goal = current_student.learning_goal;
	if (strlen(goal) > 0) {
		for (int i = 0; i < 5; i++) {
			const char *items[] = {"考研", "就业", "出国", "考公", "其他"};
			if (strcmp(goal, items[i]) == 0) {
				gtk_combo_box_set_active(GTK_COMBO_BOX(goal_combo), i);
				break;
			}
		}
	} else {
		gtk_combo_box_set_active(GTK_COMBO_BOX(goal_combo), 0);
	}
	gtk_grid_attach(GTK_GRID(grid), goal_combo, 1, 4, 1, 1);

	// 保存按钮
	GtkWidget *save_btn = gtk_button_new_with_label("保存信息");
	{ GtkStyleContext *ctx_sb = gtk_widget_get_style_context(save_btn); gtk_style_context_add_class(ctx_sb, "btn-student"); }
	g_object_set_data(G_OBJECT(save_btn), "name_entry", name_entry);
	g_object_set_data(G_OBJECT(save_btn), "major_entry", major_entry);
	g_object_set_data(G_OBJECT(save_btn), "grade_entry", grade_entry);
	g_object_set_data(G_OBJECT(save_btn), "baseline_combo", baseline_combo);
	g_object_set_data(G_OBJECT(save_btn), "goal_combo", goal_combo);
	g_signal_connect(save_btn, "clicked", G_CALLBACK(update_student_info_callback), NULL);
	gtk_box_pack_start(GTK_BOX(student_info_page), save_btn, FALSE, FALSE, 10);

	gtk_stack_add_named(GTK_STACK(stack), student_info_page, "student_info");
}

// 使用说明对话框
void show_help_callback(GtkWidget *widget, gpointer data) {
	(void)widget;
	(void)data;
	GtkWidget *dialog = gtk_dialog_new_with_buttons(
	                        "使用说明",
	                        GTK_WINDOW(main_window),
	                        GTK_DIALOG_MODAL,
	                        "确定", GTK_RESPONSE_ACCEPT,
	                        NULL);

	GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	gtk_container_set_border_width(GTK_CONTAINER(content), 20);

	GString *help_text = g_string_new(NULL);
	g_string_append(help_text, "<b><span size='large'>学习管理系统 - 使用说明</span></b>\n\n");

	g_string_append(help_text, "<b>【个人信息】</b>\n");
	g_string_append(help_text, "👤 点击\"编辑\"按钮修改个人信息\n");
	g_string_append(help_text, "📋 包括：专业、年级、学习基础、目标、薄弱方向\n\n");

	g_string_append(help_text, "<b>【课程表】</b>\n");
	g_string_append(help_text, "📅 课程表按星期显示，每天一列\n");
	g_string_append(help_text, "🕐 每列中的课程按时间自动排序\n");
	g_string_append(help_text, "📥 <b>导入课程</b>：支持CSV格式文件导入\n");
	g_string_append(help_text, "  CSV格式：课程名,时间,教室,星期\n");
	g_string_append(help_text, "📤 <b>导出课程</b>：将课程表导出为CSV文件\n");
	g_string_append(help_text, "➕ <b>添加课程</b>：手动输入课程信息添加\n\n");

	g_string_append(help_text, "<b>【学习任务】</b>\n");
	g_string_append(help_text, "✅ 设置每日学习任务\n");
	g_string_append(help_text, "👍 点击任务右侧按钮完成打卡\n");
	g_string_append(help_text, "🗑️ 已完成任务会显示删除按钮\n\n");

	g_string_append(help_text, "<b>【心情记录】</b>\n");
	g_string_append(help_text, "😊 选择当日学习状态\n");
	g_string_append(help_text, "🏷️ 状态类型：高效、突击、放松、其他\n\n");

	g_string_append(help_text, "<b>【总结图表】</b>\n");
	g_string_append(help_text, "📊 查看本周学习成果统计\n");
	g_string_append(help_text, "🥧 扇形图显示心情分布\n\n");

	g_string_append(help_text, "<b>【错题集】</b>\n");
	g_string_append(help_text, "📚 导入错题图片\n");
	g_string_append(help_text, "🖼️ 支持JPG、PNG等常见图片格式\n");
	g_string_append(help_text, "🏷️ 可按科目和难度分类\n\n");

	g_string_append(help_text, "<b>【数据保存】</b>\n");
	g_string_append(help_text, "💾 所有数据自动保存到 study_data.txt\n");
	g_string_append(help_text, "⚠️ 请勿手动修改数据文件\n\n");

	g_string_append(help_text, "<span size='small'>如有问题，请联系管理员</span>");

	GtkWidget *label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), help_text->str);
	gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
	gtk_widget_set_size_request(label, 500, -1);

	gtk_box_pack_start(GTK_BOX(content), label, TRUE, TRUE, 0);

	gtk_widget_show_all(dialog);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);

	g_string_free(help_text, TRUE);
}

// 查看错题图片回调
void view_error_question_callback(GtkWidget *widget, gpointer data) {
	(void)data; // 忽略未使用参数

	// 获取错题列表控件（数据直接存储在按钮上）
	GtkWidget *error_list = g_object_get_data(G_OBJECT(widget), "error_list");

	if (!error_list) {
		GtkWidget *error_dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
		                          GTK_DIALOG_MODAL,
		                          GTK_MESSAGE_ERROR,
		                          GTK_BUTTONS_OK,
		                          "无法获取错题列表！");
		gtk_dialog_run(GTK_DIALOG(error_dialog));
		gtk_widget_destroy(error_dialog);
		return;
	}

	// 获取选中的行
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(error_list));
	GtkTreeModel *model;
	GtkTreeIter iter;

	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		// 获取路径
		gchar *path;
		gtk_tree_model_get(model, &iter, 0, &path, -1);

		if (path && strlen(path) > 0) {
			// 使用GIO检查文件是否存在且可读
			if (!g_file_test(path, G_FILE_TEST_EXISTS) || !g_file_test(path, G_FILE_TEST_IS_REGULAR)) {
				GtkWidget *error_dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
				                          GTK_DIALOG_MODAL,
				                          GTK_MESSAGE_ERROR,
				                          GTK_BUTTONS_OK,
				                          "图片文件不存在或无法访问！");
				gtk_dialog_run(GTK_DIALOG(error_dialog));
				gtk_widget_destroy(error_dialog);
				g_free(path);
				return;
			}

			// 使用ShellExecuteW打开图片（支持Unicode路径）
			wchar_t wpath[512];
			MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, sizeof(wpath) / sizeof(wchar_t));

			HINSTANCE result = ShellExecuteW(NULL, L"open", wpath, NULL, NULL, SW_SHOWNORMAL);

			if ((INT_PTR)result <= 32) {
				GtkWidget *error_dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
				                          GTK_DIALOG_MODAL,
				                          GTK_MESSAGE_ERROR,
				                          GTK_BUTTONS_OK,
				                          "无法打开图片！错误代码: %d", (int)(INT_PTR)result);
				gtk_dialog_run(GTK_DIALOG(error_dialog));
				gtk_widget_destroy(error_dialog);
			}
		}
		g_free(path);
	} else {
		GtkWidget *error_dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
		                          GTK_DIALOG_MODAL,
		                          GTK_MESSAGE_ERROR,
		                          GTK_BUTTONS_OK,
		                          "请先选择一条错题！");
		gtk_dialog_run(GTK_DIALOG(error_dialog));
		gtk_widget_destroy(error_dialog);
	}
}

// 删除错题回调函数
void delete_error_question_callback(GtkWidget *widget, gpointer data) {
	(void)data;

	GtkWidget *error_list = g_object_get_data(G_OBJECT(widget), "error_list");
	if (!error_list) {
		GtkWidget *error_dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
		                          GTK_DIALOG_MODAL,
		                          GTK_MESSAGE_ERROR,
		                          GTK_BUTTONS_OK,
		                          "无法获取错题列表！");
		gtk_dialog_run(GTK_DIALOG(error_dialog));
		gtk_widget_destroy(error_dialog);
		return;
	}

	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(error_list));
	GtkTreeModel *model;
	GtkTreeIter iter;

	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		gchar *path = NULL;
		gtk_tree_model_get(model, &iter, 0, &path, -1);

		if (!path || strlen(path) == 0) {
			GtkWidget *error_dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
			                          GTK_DIALOG_MODAL,
			                          GTK_MESSAGE_ERROR,
			                          GTK_BUTTONS_OK,
			                          "无法获取错题路径！");
			gtk_dialog_run(GTK_DIALOG(error_dialog));
			gtk_widget_destroy(error_dialog);
			g_free(path);
			return;
		}

		GtkWidget *confirm_dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
		                            GTK_DIALOG_MODAL,
		                            GTK_MESSAGE_WARNING,
		                            GTK_BUTTONS_YES_NO,
		                            "确定要删除这条错题吗？\n路径: %s", path);
		gint response = gtk_dialog_run(GTK_DIALOG(confirm_dialog));
		gtk_widget_destroy(confirm_dialog);

		if (response == GTK_RESPONSE_YES) {
			gboolean found = FALSE;
			for (int i = 0; i < error_count; i++) {
				if (strcmp(error_questions[i].question_path, path) == 0) {
					for (int j = i; j < error_count - 1; j++) {
						error_questions[j] = error_questions[j + 1];
					}
					error_count--;
					update_error_store();
					save_data();

					GtkWidget *success_dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
					                            GTK_DIALOG_MODAL,
					                            GTK_MESSAGE_INFO,
					                            GTK_BUTTONS_OK,
					                            "错题删除成功！");
					gtk_dialog_run(GTK_DIALOG(success_dialog));
					gtk_widget_destroy(success_dialog);

					found = TRUE;
					break;
				}
			}

			if (!found) {
				GtkWidget *error_dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
				                          GTK_DIALOG_MODAL,
				                          GTK_MESSAGE_ERROR,
				                          GTK_BUTTONS_OK,
				                          "未找到匹配的错题记录！");
				gtk_dialog_run(GTK_DIALOG(error_dialog));
				gtk_widget_destroy(error_dialog);
			}
		}

		g_free(path);
	} else {
		GtkWidget *error_dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
		                          GTK_DIALOG_MODAL,
		                          GTK_MESSAGE_ERROR,
		                          GTK_BUTTONS_OK,
		                          "请先选择一条错题！");
		gtk_dialog_run(GTK_DIALOG(error_dialog));
		gtk_widget_destroy(error_dialog);
	}
}

// 显示课程页面 - 优化为按星期显示的表格形式
void show_course_page() {
	// 如果页面已存在，只需切换显示并刷新数据
	if (course_page != NULL) {
		refresh_course_page();
		gtk_stack_set_visible_child(GTK_STACK(stack), course_page);
		return;
	}

	course_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
	gtk_widget_set_margin_start(course_page, 10);
	gtk_widget_set_margin_end(course_page, 10);
	gtk_widget_set_margin_top(course_page, 10);
	gtk_widget_set_margin_bottom(course_page, 10);

	// 标题栏
	GtkWidget *header_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_box_pack_start(GTK_BOX(course_page), header_box, FALSE, FALSE, 5);

	GtkWidget *title = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(title), "<span size='xx-large' weight='bold'>我的课程表</span>");
	gtk_box_pack_start(GTK_BOX(header_box), title, TRUE, TRUE, 0);

	// 操作按钮栏
	GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_box_pack_start(GTK_BOX(course_page), button_box, FALSE, FALSE, 5);

	GtkWidget *import_btn = gtk_button_new_with_label("导入课程");
	g_signal_connect(import_btn, "clicked", G_CALLBACK(import_course_callback), NULL);
	gtk_box_pack_start(GTK_BOX(button_box), import_btn, FALSE, FALSE, 2);

	GtkWidget *export_btn = gtk_button_new_with_label("导出课程");
	g_signal_connect(export_btn, "clicked", G_CALLBACK(export_course_callback), NULL);
	gtk_box_pack_start(GTK_BOX(button_box), export_btn, FALSE, FALSE, 2);

	// 课程表格 - 7列显示（周一到周日）
	GtkWidget *table_frame = gtk_frame_new("本周课程安排");
	gtk_box_pack_start(GTK_BOX(course_page), table_frame, TRUE, TRUE, 5);

	GtkWidget *table_grid = gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(table_grid), 5);
	gtk_grid_set_column_spacing(GTK_GRID(table_grid), 5);
	gtk_container_set_border_width(GTK_CONTAINER(table_grid), 10);
	gtk_container_add(GTK_CONTAINER(table_frame), table_grid);

	// 星期标题行
	const char *days[] = {"周一", "周二", "周三", "周四", "周五", "周六", "周日"};
	GtkWidget *time_header = gtk_label_new("时间");
	gtk_widget_set_size_request(time_header, 80, -1);
	gtk_grid_attach(GTK_GRID(table_grid), time_header, 0, 0, 1, 1);

	for (int i = 0; i < 7; i++) {
		GtkWidget *day_label = gtk_label_new(NULL);
		char markup[50];
		snprintf(markup, sizeof(markup), "<b>%s</b>", days[i]);
		gtk_label_set_markup(GTK_LABEL(day_label), markup);
		gtk_widget_set_size_request(day_label, 120, -1);
		gtk_grid_attach(GTK_GRID(table_grid), day_label, i + 1, 0, 1, 1);
	}

	// 收集每天的课程并按时间排序
	GtkWidget *day_columns[7];
	for (int day = 0; day < 7; day++) {
		// 创建垂直容器存储每天的课程
		day_columns[day] = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
		gtk_widget_set_size_request(day_columns[day], 120, -1);
		gtk_grid_attach(GTK_GRID(table_grid), day_columns[day], day + 1, 1, 1, 1);

		// 收集该天的所有课程
		int day_course_count = 0;
		for (int i = 0; i < course_count; i++) {
			if (courses[i].day == day) {
				day_course_count++;
			}
		}

		// 如果没有课程，显示提示
		if (day_course_count == 0) {
			GtkWidget *empty_label = gtk_label_new("暂无课程");
			gtk_widget_set_size_request(empty_label, 120, 50);
			gtk_box_pack_start(GTK_BOX(day_columns[day]), empty_label, FALSE, FALSE, 0);
		} else {
			// 简单排序（按时间字符串比较）
			// 先显示所有课程，后续可优化为更精确的时间排序
			for (int i = 0; i < course_count; i++) {
				if (courses[i].day == day) {
					// 创建课程卡片
					GtkWidget *course_card = gtk_frame_new(NULL);
					{ GtkStyleContext *ctx_cc = gtk_widget_get_style_context(course_card); gtk_style_context_add_class(ctx_cc, "course-card"); }

					GtkWidget *course_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
					gtk_widget_set_margin_start(course_box, 6);
					gtk_widget_set_margin_end(course_box, 6);
					gtk_widget_set_margin_top(course_box, 5);
					gtk_widget_set_margin_bottom(course_box, 5);
					gtk_container_add(GTK_CONTAINER(course_card), course_box);

					// 课程名
					GtkWidget *name_label = gtk_label_new(courses[i].course_name);
					{ GtkStyleContext *ctx_nl = gtk_widget_get_style_context(name_label); gtk_style_context_add_class(ctx_nl, "course-card-name"); }
					gtk_label_set_line_wrap(GTK_LABEL(name_label), TRUE);
					gtk_label_set_justify(GTK_LABEL(name_label), GTK_JUSTIFY_CENTER);
					gtk_box_pack_start(GTK_BOX(course_box), name_label, FALSE, FALSE, 0);

					// 时间
					GtkWidget *time_label = gtk_label_new(courses[i].time);
					{ GtkStyleContext *ctx_tl = gtk_widget_get_style_context(time_label); gtk_style_context_add_class(ctx_tl, "course-card-time"); }
					gtk_label_set_justify(GTK_LABEL(time_label), GTK_JUSTIFY_CENTER);
					gtk_widget_set_size_request(time_label, 110, -1);
					gtk_box_pack_start(GTK_BOX(course_box), time_label, FALSE, FALSE, 0);

					// 教室
					if (strlen(courses[i].classroom) > 0) {
						GtkWidget *room_label = gtk_label_new(courses[i].classroom);
						{ GtkStyleContext *ctx_rl = gtk_widget_get_style_context(room_label); gtk_style_context_add_class(ctx_rl, "course-card-room"); }
						gtk_label_set_justify(GTK_LABEL(room_label), GTK_JUSTIFY_CENTER);
						gtk_box_pack_start(GTK_BOX(course_box), room_label, FALSE, FALSE, 0);
					}

					gtk_box_pack_start(GTK_BOX(day_columns[day]), course_card, FALSE, FALSE, 2);
				}
			}
		}
	}

	// 添加课程表单
	GtkWidget *form_frame = gtk_frame_new("添加新课程");
	gtk_box_pack_start(GTK_BOX(course_page), form_frame, FALSE, FALSE, 5);

	GtkWidget *form_grid = gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(form_grid), 5);
	gtk_grid_set_column_spacing(GTK_GRID(form_grid), 5);
	gtk_container_set_border_width(GTK_CONTAINER(form_grid), 10);
	gtk_container_add(GTK_CONTAINER(form_frame), form_grid);

	GtkWidget *name_label = gtk_label_new("课程名:");
	{ GtkStyleContext *ctx = gtk_widget_get_style_context(name_label); gtk_style_context_add_class(ctx, "form-label"); }
	gtk_grid_attach(GTK_GRID(form_grid), name_label, 0, 0, 1, 1);
	GtkWidget *name_entry = gtk_entry_new();
	gtk_grid_attach(GTK_GRID(form_grid), name_entry, 1, 0, 1, 1);

	GtkWidget *time_label = gtk_label_new("时间:");
	{ GtkStyleContext *ctx = gtk_widget_get_style_context(time_label); gtk_style_context_add_class(ctx, "form-label"); }
	gtk_grid_attach(GTK_GRID(form_grid), time_label, 2, 0, 1, 1);
	GtkWidget *time_entry = gtk_entry_new();
	gtk_grid_attach(GTK_GRID(form_grid), time_entry, 3, 0, 1, 1);

	GtkWidget *classroom_label = gtk_label_new("教室:");
	{ GtkStyleContext *ctx = gtk_widget_get_style_context(classroom_label); gtk_style_context_add_class(ctx, "form-label"); }
	gtk_grid_attach(GTK_GRID(form_grid), classroom_label, 0, 1, 1, 1);
	GtkWidget *classroom_entry = gtk_entry_new();
	gtk_grid_attach(GTK_GRID(form_grid), classroom_entry, 1, 1, 1, 1);

	GtkWidget *day_label = gtk_label_new("星期:");
	{ GtkStyleContext *ctx = gtk_widget_get_style_context(day_label); gtk_style_context_add_class(ctx, "form-label"); }
	gtk_grid_attach(GTK_GRID(form_grid), day_label, 2, 1, 1, 1);
	GtkWidget *day_combo = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(day_combo), "周一");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(day_combo), "周二");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(day_combo), "周三");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(day_combo), "周四");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(day_combo), "周五");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(day_combo), "周六");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(day_combo), "周日");
	gtk_combo_box_set_active(GTK_COMBO_BOX(day_combo), 0);
	gtk_grid_attach(GTK_GRID(form_grid), day_combo, 3, 1, 1, 1);

	GtkWidget *add_btn = gtk_button_new_with_label("添加课程");
	g_object_set_data(G_OBJECT(add_btn), "name_entry", name_entry);
	g_object_set_data(G_OBJECT(add_btn), "time_entry", time_entry);
	g_object_set_data(G_OBJECT(add_btn), "classroom_entry", classroom_entry);
	g_object_set_data(G_OBJECT(add_btn), "day_combo", day_combo);
	g_signal_connect(add_btn, "clicked", G_CALLBACK(add_course_callback), NULL);
	gtk_grid_attach(GTK_GRID(form_grid), add_btn, 0, 2, 4, 1);

	gtk_widget_show_all(course_page);
	gtk_stack_add_named(GTK_STACK(stack), course_page, "course");
}

// 显示任务页面
void show_task_page() {
	// 如果页面已存在，只需切换显示并更新数据
	if (task_page != NULL) {
		update_task_store();
		gtk_stack_set_visible_child(GTK_STACK(stack), task_page);
		return;
	}

	task_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
	gtk_widget_set_margin_start(task_page, 20);
	gtk_widget_set_margin_end(task_page, 20);
	gtk_widget_set_margin_top(task_page, 20);
	gtk_widget_set_margin_bottom(task_page, 20);

	GtkWidget *title = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(title), "<span size='xx-large' weight='bold'>学习任务</span>");
	gtk_box_pack_start(GTK_BOX(task_page), title, FALSE, FALSE, 10);

	// 任务统计信息
	GtkWidget *stats_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
	gtk_box_pack_start(GTK_BOX(task_page), stats_bar, FALSE, FALSE, 5);

	char task_stats[200];
	int completed_tasks = 0;
	for (int i = 0; i < task_count; i++) {
		if (tasks[i].completed)
			completed_tasks++;
	}
	snprintf(task_stats, sizeof(task_stats), "任务总数: %d | 已完成: %d | 进行中: %d",
	         task_count, completed_tasks, task_count - completed_tasks);
	GtkWidget *stats_label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(stats_label), task_stats);
	gtk_box_pack_start(GTK_BOX(stats_bar), stats_label, FALSE, FALSE, 0);

	// 任务列表
	GtkWidget *list_frame = gtk_frame_new("任务列表");
	gtk_box_pack_start(GTK_BOX(task_page), list_frame, TRUE, TRUE, 5);

	GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_size_request(scrolled, -1, 280);
	gtk_container_add(GTK_CONTAINER(list_frame), scrolled);

	GtkWidget *task_list = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(task_list), TRUE);
	gtk_container_add(GTK_CONTAINER(scrolled), task_list);

	// 使用全局store，避免重复创建
	task_store = gtk_list_store_new(5, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_STRING);
	gtk_tree_view_set_model(GTK_TREE_VIEW(task_list), GTK_TREE_MODEL(task_store));

	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	GtkCellRenderer *toggle_renderer = gtk_cell_renderer_toggle_new();
	g_object_set(toggle_renderer, "activatable", TRUE, "indicator-size", 20, NULL);
	g_signal_connect(toggle_renderer, "toggled", G_CALLBACK(task_toggle_callback), task_store);

	GtkTreeViewColumn *col1 = gtk_tree_view_column_new_with_attributes("任务名称", renderer, "text", 0, NULL);
	GtkTreeViewColumn *col2 = gtk_tree_view_column_new_with_attributes("截止日期", renderer, "text", 1, NULL);
	GtkTreeViewColumn *col3 = gtk_tree_view_column_new_with_attributes("任务描述", renderer, "text", 2, NULL);
	GtkTreeViewColumn *col4 = gtk_tree_view_column_new_with_attributes("完成状态", toggle_renderer, "active", 3, NULL);
	GtkTreeViewColumn *col5 = gtk_tree_view_column_new_with_attributes("预计时长", renderer, "text", 4, NULL);

	gtk_tree_view_column_set_resizable(GTK_TREE_VIEW_COLUMN(col1), TRUE);
	gtk_tree_view_column_set_resizable(GTK_TREE_VIEW_COLUMN(col2), TRUE);
	gtk_tree_view_column_set_resizable(GTK_TREE_VIEW_COLUMN(col3), TRUE);
	gtk_tree_view_column_set_resizable(GTK_TREE_VIEW_COLUMN(col4), TRUE);
	gtk_tree_view_column_set_resizable(GTK_TREE_VIEW_COLUMN(col5), TRUE);

	gtk_tree_view_column_set_min_width(GTK_TREE_VIEW_COLUMN(col1), 120);
	gtk_tree_view_column_set_min_width(GTK_TREE_VIEW_COLUMN(col2), 100);
	gtk_tree_view_column_set_min_width(GTK_TREE_VIEW_COLUMN(col3), 180);
	gtk_tree_view_column_set_min_width(GTK_TREE_VIEW_COLUMN(col4), 80);
	gtk_tree_view_column_set_min_width(GTK_TREE_VIEW_COLUMN(col5), 80);

	gtk_tree_view_append_column(GTK_TREE_VIEW(task_list), col1);
	gtk_tree_view_append_column(GTK_TREE_VIEW(task_list), col2);
	gtk_tree_view_append_column(GTK_TREE_VIEW(task_list), col3);
	gtk_tree_view_append_column(GTK_TREE_VIEW(task_list), col4);
	gtk_tree_view_append_column(GTK_TREE_VIEW(task_list), col5);

	// 添加新任务
	GtkWidget *form_frame = gtk_frame_new("添加任务");
	gtk_box_pack_start(GTK_BOX(task_page), form_frame, FALSE, FALSE, 5);

	GtkWidget *form_grid = gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(form_grid), 8);
	gtk_grid_set_column_spacing(GTK_GRID(form_grid), 10);
	gtk_container_set_border_width(GTK_CONTAINER(form_grid), 10);
	gtk_container_add(GTK_CONTAINER(form_frame), form_grid);

	GtkWidget *name_label = gtk_label_new("任务名称:");
	{ GtkStyleContext *ctx = gtk_widget_get_style_context(name_label); gtk_style_context_add_class(ctx, "form-label"); }
	gtk_widget_set_size_request(name_label, 80, -1);
	gtk_grid_attach(GTK_GRID(form_grid), name_label, 0, 0, 1, 1);
	GtkWidget *name_entry = gtk_entry_new();
	gtk_widget_set_size_request(name_entry, 200, -1);
	gtk_entry_set_placeholder_text(GTK_ENTRY(name_entry), "请输入任务名称");
	gtk_grid_attach(GTK_GRID(form_grid), name_entry, 1, 0, 1, 1);

	GtkWidget *deadline_label = gtk_label_new("截止日期:");
	{ GtkStyleContext *ctx = gtk_widget_get_style_context(deadline_label); gtk_style_context_add_class(ctx, "form-label"); }
	gtk_widget_set_size_request(deadline_label, 80, -1);
	gtk_grid_attach(GTK_GRID(form_grid), deadline_label, 2, 0, 1, 1);
	GtkWidget *deadline_entry = gtk_entry_new();
	gtk_widget_set_size_request(deadline_entry, 150, -1);
	gtk_entry_set_placeholder_text(GTK_ENTRY(deadline_entry), "YYYY-MM-DD");
	gtk_grid_attach(GTK_GRID(form_grid), deadline_entry, 3, 0, 1, 1);

	GtkWidget *desc_label = gtk_label_new("任务描述:");
	{ GtkStyleContext *ctx = gtk_widget_get_style_context(desc_label); gtk_style_context_add_class(ctx, "form-label"); }
	gtk_widget_set_size_request(desc_label, 80, -1);
	gtk_grid_attach(GTK_GRID(form_grid), desc_label, 0, 1, 1, 1);
	GtkWidget *desc_entry = gtk_entry_new();
	gtk_widget_set_size_request(desc_entry, 250, -1);
	gtk_entry_set_placeholder_text(GTK_ENTRY(desc_entry), "请输入任务描述（可选）");
	gtk_grid_attach(GTK_GRID(form_grid), desc_entry, 1, 1, 1, 1);

	GtkWidget *duration_label = gtk_label_new("预计时长:");
	{ GtkStyleContext *ctx = gtk_widget_get_style_context(duration_label); gtk_style_context_add_class(ctx, "form-label"); }
	gtk_widget_set_size_request(duration_label, 80, -1);
	gtk_grid_attach(GTK_GRID(form_grid), duration_label, 2, 1, 1, 1);
	GtkWidget *duration_entry = gtk_entry_new();
	gtk_widget_set_size_request(duration_entry, 80, -1);
	gtk_entry_set_placeholder_text(GTK_ENTRY(duration_entry), "分钟");
	gtk_grid_attach(GTK_GRID(form_grid), duration_entry, 3, 1, 1, 1);

	GtkWidget *add_btn = gtk_button_new_with_label("添加任务");
	gtk_widget_set_size_request(add_btn, 100, -1);
	g_object_set_data(G_OBJECT(add_btn), "name_entry", name_entry);
	g_object_set_data(G_OBJECT(add_btn), "deadline_entry", deadline_entry);
	g_object_set_data(G_OBJECT(add_btn), "desc_entry", desc_entry);
	g_object_set_data(G_OBJECT(add_btn), "duration_entry", duration_entry);
	g_signal_connect(add_btn, "clicked", G_CALLBACK(add_task_callback), NULL);
	gtk_grid_attach(GTK_GRID(form_grid), add_btn, 0, 2, 4, 1);

	// 填充初始数据
	update_task_store();

	gtk_stack_add_named(GTK_STACK(stack), task_page, "task");
}

// 显示心情页面
void show_mood_page() {
	// 如果页面已存在，只需切换显示并更新数据
	if (mood_page != NULL) {
		update_mood_store();
		gtk_stack_set_visible_child(GTK_STACK(stack), mood_page);
		return;
	}

	mood_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
	gtk_widget_set_margin_start(mood_page, 20);
	gtk_widget_set_margin_end(mood_page, 20);
	gtk_widget_set_margin_top(mood_page, 20);
	gtk_widget_set_margin_bottom(mood_page, 20);

	GtkWidget *title = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(title), "<span size='xx-large' weight='bold'>心情记录</span>");
	gtk_box_pack_start(GTK_BOX(mood_page), title, FALSE, FALSE, 10);

	// 心情统计信息
	GtkWidget *stats_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
	gtk_box_pack_start(GTK_BOX(mood_page), stats_bar, FALSE, FALSE, 5);

	char mood_stats[200];
	snprintf(mood_stats, sizeof(mood_stats), "心情记录总数: %d 条", mood_count);
	if (mood_count > 0) {
		strncat(mood_stats, " | ", sizeof(mood_stats) - strlen(mood_stats) - 1);
		strncat(mood_stats, "今日状态: ", sizeof(mood_stats) - strlen(mood_stats) - 1);
		strncat(mood_stats, mood_logs[mood_count - 1].status, sizeof(mood_stats) - strlen(mood_stats) - 1);
	}
	GtkWidget *stats_label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(stats_label), mood_stats);
	gtk_box_pack_start(GTK_BOX(stats_bar), stats_label, FALSE, FALSE, 0);

	// 心情列表
	GtkWidget *list_frame = gtk_frame_new("心情历史记录");
	gtk_box_pack_start(GTK_BOX(mood_page), list_frame, TRUE, TRUE, 5);

	GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_size_request(scrolled, -1, 250);
	gtk_container_add(GTK_CONTAINER(list_frame), scrolled);

	GtkWidget *mood_list = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(mood_list), TRUE);
	gtk_container_add(GTK_CONTAINER(scrolled), mood_list);

	// 使用全局store，避免重复创建
	mood_store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
	gtk_tree_view_set_model(GTK_TREE_VIEW(mood_list), GTK_TREE_MODEL(mood_store));

	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	GtkTreeViewColumn *col1 = gtk_tree_view_column_new_with_attributes("日期", renderer, "text", 0, NULL);
	GtkTreeViewColumn *col2 = gtk_tree_view_column_new_with_attributes("学习状态", renderer, "text", 1, NULL);

	gtk_tree_view_column_set_resizable(GTK_TREE_VIEW_COLUMN(col1), TRUE);
	gtk_tree_view_column_set_resizable(GTK_TREE_VIEW_COLUMN(col2), TRUE);
	gtk_tree_view_column_set_min_width(GTK_TREE_VIEW_COLUMN(col1), 120);
	gtk_tree_view_column_set_min_width(GTK_TREE_VIEW_COLUMN(col2), 100);

	gtk_tree_view_append_column(GTK_TREE_VIEW(mood_list), col1);
	gtk_tree_view_append_column(GTK_TREE_VIEW(mood_list), col2);

	// 状态选择表单
	GtkWidget *form_frame = gtk_frame_new("记录今日学习状态");
	gtk_box_pack_start(GTK_BOX(mood_page), form_frame, FALSE, FALSE, 5);

	GtkWidget *form_grid = gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(form_grid), 10);
	gtk_grid_set_column_spacing(GTK_GRID(form_grid), 10);
	gtk_container_set_border_width(GTK_CONTAINER(form_grid), 15);
	gtk_container_add(GTK_CONTAINER(form_frame), form_grid);

	GtkWidget *status_label = gtk_label_new("选择今日学习状态:");
	{ GtkStyleContext *ctx = gtk_widget_get_style_context(status_label); gtk_style_context_add_class(ctx, "form-label"); }
	gtk_widget_set_size_request(status_label, 120, -1);
	gtk_grid_attach(GTK_GRID(form_grid), status_label, 0, 0, 1, 1);

	GtkWidget *status_combo = gtk_combo_box_text_new();
	gtk_widget_set_size_request(status_combo, 150, -1);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(status_combo), "高效");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(status_combo), "突击");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(status_combo), "放松");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(status_combo), "迷茫");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(status_combo), "疲惫");
	gtk_combo_box_set_active(GTK_COMBO_BOX(status_combo), 0);
	gtk_grid_attach(GTK_GRID(form_grid), status_combo, 1, 0, 1, 1);

	GtkWidget *set_btn = gtk_button_new_with_label("记录心情");
	gtk_widget_set_size_request(set_btn, 100, -1);
	g_object_set_data(G_OBJECT(set_btn), "status_combo", status_combo);
	g_signal_connect(set_btn, "clicked", G_CALLBACK(set_mood_callback), NULL);
	gtk_grid_attach(GTK_GRID(form_grid), set_btn, 0, 1, 2, 1);

	// 状态说明
	GtkWidget *tips_label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(tips_label),
	                     "<span size='small'>状态说明: 高效(专注学习) | 突击(赶工模式) | 放松(轻松学习) | 迷茫(不知学啥) | 疲惫(需要休息)</span>");
	gtk_label_set_line_wrap(GTK_LABEL(tips_label), TRUE);
	gtk_grid_attach(GTK_GRID(form_grid), tips_label, 0, 2, 2, 1);

	// 推荐学习任务按钮
	GtkWidget *recommend_btn = gtk_button_new_with_label("获取学习任务推荐");
	gtk_widget_set_size_request(recommend_btn, 180, -1);
	g_signal_connect(recommend_btn, "clicked", G_CALLBACK(recommend_task_callback), NULL);
	gtk_grid_attach(GTK_GRID(form_grid), recommend_btn, 0, 3, 2, 1);

	// 填充初始数据
	update_mood_store();

	gtk_stack_add_named(GTK_STACK(stack), mood_page, "mood");
}

// 显示统计图表页面
void show_chart_page() {
	// 如果页面已存在，只需切换显示并刷新图表
	if (chart_page != NULL) {
		refresh_chart();
		gtk_stack_set_visible_child(GTK_STACK(stack), chart_page);
		return;
	}

	chart_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
	gtk_widget_set_margin_start(chart_page, 20);
	gtk_widget_set_margin_end(chart_page, 20);
	gtk_widget_set_margin_top(chart_page, 20);
	gtk_widget_set_margin_bottom(chart_page, 20);

	GtkWidget *title = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(title), "<span size='xx-large' weight='bold'>学习统计</span>");
	gtk_box_pack_start(GTK_BOX(chart_page), title, FALSE, FALSE, 10);

	// 图表和统计概况放在同一行
	GtkWidget *main_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
	gtk_box_pack_start(GTK_BOX(chart_page), main_hbox, TRUE, TRUE, 5);

	// 图表区域 - 使用全局引用
	GtkWidget *chart_frame = gtk_frame_new("今日任务完成时间占比");
	{ GtkStyleContext *ctx_cf = gtk_widget_get_style_context(chart_frame); gtk_style_context_add_class(ctx_cf, "card-frame"); gtk_style_context_add_class(ctx_cf, "card-frame-chart"); }
	gtk_box_pack_start(GTK_BOX(main_hbox), chart_frame, TRUE, TRUE, 5);

	chart_area = gtk_drawing_area_new();
	gtk_widget_set_size_request(chart_area, 350, 300);
	g_signal_connect(chart_area, "draw", G_CALLBACK(draw_chart_callback), NULL);
	gtk_container_add(GTK_CONTAINER(chart_frame), chart_area);

	// 统计信息
	GtkWidget *stats_frame = gtk_frame_new("学习统计概览");
	gtk_widget_set_size_request(stats_frame, 300, -1);
	gtk_box_pack_start(GTK_BOX(main_hbox), stats_frame, FALSE, FALSE, 5);

	GtkWidget *stats_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	gtk_container_set_border_width(GTK_CONTAINER(stats_vbox), 10);
	gtk_container_add(GTK_CONTAINER(stats_frame), stats_vbox);

	char stats_text[800];
	int completed_tasks = 0, pending_tasks = 0;

	for (int i = 0; i < task_count; i++) {
		if (tasks[i].completed)
			completed_tasks++;
		else
			pending_tasks++;
	}

	double completion_rate = task_count > 0 ? (double)completed_tasks / task_count * 100 : 0;

	snprintf(stats_text, sizeof(stats_text),
	         "<b>课程信息</b>\n"
	         "  📅 课程总数: %d门\n\n"
	         "<b>任务信息</b>\n"
	         "  📋 任务总数: %d个\n"
	         "  ✅ 已完成: %d个\n"
	         "  ⏳ 待完成: %d个\n"
	         "  📊 完成率: <span color='green'>%.1f%%</span>\n\n"
	         "<b>心情记录</b>\n"
	         "  📝 记录总数: %d条\n",
	         course_count,
	         task_count, completed_tasks, pending_tasks, completion_rate,
	         mood_count);

	if (mood_count > 0) {
		char mood_info[200];
		snprintf(mood_info, sizeof(mood_info),
		         "  😊 今日心情: <span color='blue'>%s</span>\n",
		         mood_logs[mood_count - 1].status);
		strncat(stats_text, mood_info, sizeof(stats_text) - strlen(stats_text) - 1);
	}

	strncat(stats_text, "\n<b>错题信息</b>\n", sizeof(stats_text) - strlen(stats_text) - 1);
	char error_info[200];
	snprintf(error_info, sizeof(error_info),
	         "  📚 错题总数: %d道\n", error_count);
	strncat(stats_text, error_info, sizeof(stats_text) - strlen(stats_text) - 1);

	GtkWidget *stats_label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(stats_label), stats_text);
	gtk_label_set_justify(GTK_LABEL(stats_label), GTK_JUSTIFY_LEFT);
	gtk_box_pack_start(GTK_BOX(stats_vbox), stats_label, FALSE, FALSE, 5);

	gtk_stack_add_named(GTK_STACK(stack), chart_page, "chart");
}

// 显示错题页面
void show_error_page() {
	// 如果页面已存在，只需切换显示并更新数据
	if (error_page != NULL) {
		update_error_store();
		// 强制更新科目列表 - 确保每次切换到错题页面都刷新
		if (error_subject_combo) {
			// 清空并重新填充
			gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(error_subject_combo));
			if (course_count == 0) {
				gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(error_subject_combo), "暂无课程，请先添加课程");
				gtk_combo_box_set_active(GTK_COMBO_BOX(error_subject_combo), 0);
			} else {
				for (int i = 0; i < course_count; i++) {
					int dup = 0;
					for (int j = 0; j < i; j++) {
						if (strcmp(courses[i].course_name, courses[j].course_name) == 0) {
							dup = 1;
							break;
						}
					}
					if (!dup)
						gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(error_subject_combo), courses[i].course_name);
				}
				gtk_combo_box_set_active(GTK_COMBO_BOX(error_subject_combo), 0);
			}
			// 强制刷新
			gtk_widget_hide(error_subject_combo);
			gtk_widget_show(error_subject_combo);
		}
		gtk_stack_set_visible_child(GTK_STACK(stack), error_page);
		return;
	}

	error_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
	gtk_widget_set_margin_start(error_page, 24);
	gtk_widget_set_margin_end(error_page, 24);
	gtk_widget_set_margin_top(error_page, 16);
	gtk_widget_set_margin_bottom(error_page, 16);

	GtkWidget *title = gtk_label_new("错题集");
	{
		GtkStyleContext *ctx_t = gtk_widget_get_style_context(title);
		gtk_style_context_add_class(ctx_t, "page-title");
		gtk_style_context_add_class(ctx_t, "page-title-error");
	}
	gtk_box_pack_start(GTK_BOX(error_page), title, FALSE, FALSE, 8);

	// 错题列表
	GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_size_request(scrolled, -1, 250);
	gtk_box_pack_start(GTK_BOX(error_page), scrolled, TRUE, TRUE, 5);

	GtkWidget *error_list = gtk_tree_view_new();
	gtk_container_add(GTK_CONTAINER(scrolled), error_list);

	// 设置选择模式为单选
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(error_list));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);

	// 使用全局store，避免重复创建
	error_store = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	gtk_tree_view_set_model(GTK_TREE_VIEW(error_list), GTK_TREE_MODEL(error_store));

	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	GtkTreeViewColumn *col1 = gtk_tree_view_column_new_with_attributes("题目路径", renderer, "text", 0, NULL);
	GtkTreeViewColumn *col2 = gtk_tree_view_column_new_with_attributes("科目", renderer, "text", 1, NULL);
	GtkTreeViewColumn *col3 = gtk_tree_view_column_new_with_attributes("难度", renderer, "text", 2, NULL);

	gtk_tree_view_append_column(GTK_TREE_VIEW(error_list), col1);
	gtk_tree_view_append_column(GTK_TREE_VIEW(error_list), col2);
	gtk_tree_view_append_column(GTK_TREE_VIEW(error_list), col3);

	// 操作按钮栏
	GtkWidget *btn_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_box_pack_start(GTK_BOX(error_page), btn_hbox, FALSE, FALSE, 5);

	// 添加查看图片按钮
	GtkWidget *view_btn = gtk_button_new_with_label("查看选中图片");
	{ GtkStyleContext *ctx_vb = gtk_widget_get_style_context(view_btn); gtk_style_context_add_class(ctx_vb, "btn-error"); }
	g_object_set_data(G_OBJECT(view_btn), "error_list", error_list);
	g_signal_connect(view_btn, "clicked", G_CALLBACK(view_error_question_callback), NULL);
	gtk_box_pack_start(GTK_BOX(btn_hbox), view_btn, FALSE, FALSE, 5);

	// 添加删除错题按钮
	GtkWidget *delete_btn = gtk_button_new_with_label("删除选中错题");
	{ GtkStyleContext *ctx_db = gtk_widget_get_style_context(delete_btn); gtk_style_context_add_class(ctx_db, "btn-error"); }
	g_object_set_data(G_OBJECT(delete_btn), "error_list", error_list);
	g_signal_connect(delete_btn, "clicked", G_CALLBACK(delete_error_question_callback), NULL);
	gtk_box_pack_start(GTK_BOX(btn_hbox), delete_btn, FALSE, FALSE, 5);

	// 添加错题表单
	GtkWidget *form_frame = gtk_frame_new("添加错题");
	{ GtkStyleContext *ctx_ff = gtk_widget_get_style_context(form_frame); gtk_style_context_add_class(ctx_ff, "card-frame"); gtk_style_context_add_class(ctx_ff, "card-frame-error"); }
	gtk_box_pack_start(GTK_BOX(error_page), form_frame, FALSE, FALSE, 5);

	GtkWidget *form_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	gtk_container_add(GTK_CONTAINER(form_frame), form_vbox);

	GtkWidget *select_btn = gtk_button_new_with_label("选择题目图片");
	{ GtkStyleContext *ctx_sb2 = gtk_widget_get_style_context(select_btn); gtk_style_context_add_class(ctx_sb2, "btn-error"); }
	gtk_box_pack_start(GTK_BOX(form_vbox), select_btn, FALSE, FALSE, 5);

	GtkWidget *subject_label = gtk_label_new("选择科目:");
	{ GtkStyleContext *ctx = gtk_widget_get_style_context(subject_label); gtk_style_context_add_class(ctx, "form-label"); }
	gtk_box_pack_start(GTK_BOX(form_vbox), subject_label, FALSE, FALSE, 2);

	error_subject_combo = gtk_combo_box_text_new();
	// 初始填充课程列表
	update_error_subject_combo();
	gtk_box_pack_start(GTK_BOX(form_vbox), error_subject_combo, FALSE, FALSE, 5);

	GtkWidget *diff_label = gtk_label_new("选择难度:");
	{ GtkStyleContext *ctx = gtk_widget_get_style_context(diff_label); gtk_style_context_add_class(ctx, "form-label"); }
	gtk_box_pack_start(GTK_BOX(form_vbox), diff_label, FALSE, FALSE, 2);

	GtkWidget *diff_combo = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(diff_combo), "难");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(diff_combo), "中等");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(diff_combo), "易");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(diff_combo), "基础");
	gtk_combo_box_set_active(GTK_COMBO_BOX(diff_combo), 1);
	gtk_box_pack_start(GTK_BOX(form_vbox), diff_combo, FALSE, FALSE, 5);

	g_object_set_data(G_OBJECT(select_btn), "subject_combo", error_subject_combo);
	g_object_set_data(G_OBJECT(select_btn), "diff_combo", diff_combo);
	g_signal_connect(select_btn, "clicked", G_CALLBACK(import_error_question_callback), NULL);

	// 填充初始数据
	update_error_store();

	gtk_stack_add_named(GTK_STACK(stack), error_page, "error");
}

// 显示个人中心页面
void show_profile_page() {
	// 如果页面已存在，只需切换显示
	if (profile_page != NULL) {
		// 更新个人中心显示
		update_profile_display();
		gtk_stack_set_visible_child(GTK_STACK(stack), profile_page);
		return;
	}

	profile_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
	gtk_widget_set_margin_start(profile_page, 24);
	gtk_widget_set_margin_end(profile_page, 24);
	gtk_widget_set_margin_top(profile_page, 16);
	gtk_widget_set_margin_bottom(profile_page, 16);

	GtkWidget *title = gtk_label_new("个人中心");
	{
		GtkStyleContext *ctx_t = gtk_widget_get_style_context(title);
		gtk_style_context_add_class(ctx_t, "page-title");
		gtk_style_context_add_class(ctx_t, "page-title-profile");
	}
	gtk_box_pack_start(GTK_BOX(profile_page), title, FALSE, FALSE, 8);

	// 学生信息展示
	GtkWidget *info_frame = gtk_frame_new("学生信息");
	{ GtkStyleContext *ctx_if2 = gtk_widget_get_style_context(info_frame); gtk_style_context_add_class(ctx_if2, "card-frame"); gtk_style_context_add_class(ctx_if2, "card-frame-profile"); }
	gtk_box_pack_start(GTK_BOX(profile_page), info_frame, FALSE, FALSE, 5);

	GtkWidget *info_grid = gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(info_grid), 5);
	gtk_grid_set_column_spacing(GTK_GRID(info_grid), 10);
	gtk_container_add(GTK_CONTAINER(info_frame), info_grid);

	GtkWidget *name_label = gtk_label_new("<b>姓名:</b>");
	gtk_label_set_use_markup(GTK_LABEL(name_label), TRUE);
	gtk_grid_attach(GTK_GRID(info_grid), name_label, 0, 0, 1, 1);
	profile_name_label = gtk_label_new(current_student.name[0] ? current_student.name : "未填写");
	gtk_grid_attach(GTK_GRID(info_grid), profile_name_label, 1, 0, 1, 1);

	GtkWidget *major_label = gtk_label_new("<b>专业:</b>");
	gtk_label_set_use_markup(GTK_LABEL(major_label), TRUE);
	gtk_grid_attach(GTK_GRID(info_grid), major_label, 0, 1, 1, 1);
	profile_major_label = gtk_label_new(current_student.major[0] ? current_student.major : "未填写");
	gtk_grid_attach(GTK_GRID(info_grid), profile_major_label, 1, 1, 1, 1);

	char grade_str[20];
	snprintf(grade_str, sizeof(grade_str), "%d", current_student.grade);
	GtkWidget *grade_label = gtk_label_new("<b>年级:</b>");
	gtk_label_set_use_markup(GTK_LABEL(grade_label), TRUE);
	gtk_grid_attach(GTK_GRID(info_grid), grade_label, 0, 2, 1, 1);
	profile_grade_label = gtk_label_new(current_student.grade ? grade_str : "未填写");
	gtk_grid_attach(GTK_GRID(info_grid), profile_grade_label, 1, 2, 1, 1);

	GtkWidget *baseline_label = gtk_label_new("<b>学习基础:</b>");
	gtk_label_set_use_markup(GTK_LABEL(baseline_label), TRUE);
	gtk_grid_attach(GTK_GRID(info_grid), baseline_label, 0, 3, 1, 1);
	profile_baseline_label = gtk_label_new(current_student.learning_baseline[0] ? current_student.learning_baseline :
	                                       "未填写");
	gtk_grid_attach(GTK_GRID(info_grid), profile_baseline_label, 1, 3, 1, 1);

	GtkWidget *goal_label = gtk_label_new("<b>学习目标:</b>");
	gtk_label_set_use_markup(GTK_LABEL(goal_label), TRUE);
	gtk_grid_attach(GTK_GRID(info_grid), goal_label, 0, 4, 1, 1);
	profile_goal_label = gtk_label_new(current_student.learning_goal[0] ? current_student.learning_goal : "未填写");
	gtk_grid_attach(GTK_GRID(info_grid), profile_goal_label, 1, 4, 1, 1);

	// 统计信息
	GtkWidget *stats_frame = gtk_frame_new("学习统计");
	{ GtkStyleContext *ctx_sf2 = gtk_widget_get_style_context(stats_frame); gtk_style_context_add_class(ctx_sf2, "card-frame"); gtk_style_context_add_class(ctx_sf2, "card-frame-profile"); }
	gtk_box_pack_start(GTK_BOX(profile_page), stats_frame, FALSE, FALSE, 5);

	GtkWidget *stats_grid = gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(stats_grid), 5);
	gtk_grid_set_column_spacing(GTK_GRID(stats_grid), 10);
	gtk_container_add(GTK_CONTAINER(stats_frame), stats_grid);

	char task_stats[100], mood_stats[100], error_stats[100];
	int completed_tasks = 0;

	for (int i = 0; i < task_count; i++) {
		if (tasks[i].completed)
			completed_tasks++;
	}

	snprintf(task_stats, sizeof(task_stats), "%d/%d", completed_tasks, task_count);
	snprintf(mood_stats, sizeof(mood_stats), "%d 条记录", mood_count);
	snprintf(error_stats, sizeof(error_stats), "%d 道错题", error_count);

	GtkWidget *task_stat_label = gtk_label_new("<b>任务完成数:</b>");
	gtk_label_set_use_markup(GTK_LABEL(task_stat_label), TRUE);
	gtk_grid_attach(GTK_GRID(stats_grid), task_stat_label, 0, 0, 1, 1);
	GtkWidget *task_stat_value = gtk_label_new(task_stats);
	gtk_grid_attach(GTK_GRID(stats_grid), task_stat_value, 1, 0, 1, 1);

	GtkWidget *mood_stat_label = gtk_label_new("<b>心情记录数:</b>");
	gtk_label_set_use_markup(GTK_LABEL(mood_stat_label), TRUE);
	gtk_grid_attach(GTK_GRID(stats_grid), mood_stat_label, 0, 1, 1, 1);
	GtkWidget *mood_stat_value = gtk_label_new(mood_stats);
	gtk_grid_attach(GTK_GRID(stats_grid), mood_stat_value, 1, 1, 1, 1);

	GtkWidget *error_stat_label = gtk_label_new("<b>错题数量:</b>");
	gtk_label_set_use_markup(GTK_LABEL(error_stat_label), TRUE);
	gtk_grid_attach(GTK_GRID(stats_grid), error_stat_label, 0, 2, 1, 1);
	GtkWidget *error_stat_value = gtk_label_new(error_stats);
	gtk_grid_attach(GTK_GRID(stats_grid), error_stat_value, 1, 2, 1, 1);

	gtk_stack_add_named(GTK_STACK(stack), profile_page, "profile");
}

// 主函数
int main(int argc, char *argv[]) {
	// 初始化国际化支持
	setlocale(LC_ALL, "");          //使用系统区域设置
#ifdef _WIN32
	setlocale(LC_ALL, ".UTF-8");
	_putenv("FONTCONFIG_PATH=E:\\学习系统\\fonts");
#endif
	GtkApplication *app = gtk_application_new("com.example.study_manager", G_APPLICATION_DEFAULT_FLAGS);
	g_signal_connect(app, "activate", G_CALLBACK(create_main_window), NULL);

	int status = g_application_run(G_APPLICATION(app), argc, argv);
	g_object_unref(app);

	return status;
}
