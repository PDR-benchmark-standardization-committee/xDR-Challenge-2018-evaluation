#include<iostream>
#include<windows.h>
#include<fstream>
#include<vector>
#include<string>
#include<sstream>
#include<omp.h>
#include<algorithm>
#include<numeric>
#include<math.h>
#include<direct.h>
#include<time.h>

//Conf(Configuration file) array correspondence number
#define MAP_X 0
#define MAP_Y 1
#define MATERIAL_DNAME 2
#define OBSTACLE_FNAME 3
#define ALL_WMS_FNAME 4
#define TEST_WMS_FNAME 5
#define PDR2VDR_FNAME 6
#define SENS_START_END_FNAME 7
#define FORKLIFT_NO_INVASION_FNAME 8
#define TRA_DNAME1 9

using namespace std;

//Comma removal process of character string
vector<string> split(string& input, char delimiter) {
	istringstream stream(input);
	string field;
	vector<string> result;
	while (getline(stream, field, delimiter)) {
		result.push_back(field);
	}
	return result;
}

//Load configuration file(config.ini)
vector<string> Config() {
	cout << "Loading configuration file." << endl;

	vector<string> conf;
	char buf[1024];

	GetPrivateProfileString("map", "x", "no data", buf, sizeof(buf), ".\\config.ini");
	conf.push_back(string(buf));
	GetPrivateProfileString("map", "y", "no data", buf, sizeof(buf), ".\\config.ini");
	conf.push_back(string(buf));
	GetPrivateProfileString("material", "material_dname", "no data", buf, sizeof(buf), ".\\config.ini");
	conf.push_back(string(buf));
	GetPrivateProfileString("material", "all_obstacle_fname", "no data", buf, sizeof(buf), ".\\config.ini");
	conf.push_back(string(buf));
	GetPrivateProfileString("material", "all_wms_fname_list", "no data", buf, sizeof(buf), ".\\config.ini");
	conf.push_back(string(buf));
	GetPrivateProfileString("material", "test_wms_fname_list", "no data", buf, sizeof(buf), ".\\config.ini");
	conf.push_back(string(buf));
	GetPrivateProfileString("material", "pdr2vdr_wms", "no data", buf, sizeof(buf), ".\\config.ini");
	conf.push_back(string(buf));
	GetPrivateProfileString("material", "sens_start_end", "no data", buf, sizeof(buf), ".\\config.ini");
	conf.push_back(string(buf));
	GetPrivateProfileString("material", "forklift_no_invasion", "no data", buf, sizeof(buf), ".\\config.ini");
	conf.push_back(string(buf));

	GetPrivateProfileSection("trajectory", buf, sizeof(buf), ".\\config.ini");
	bool dname_frag = false;
	string dname = "";
	int count = 0;
	for (int i = 0; i < sizeof(buf); i++) {

		if (buf[i] == '\0') {
			dname_frag = false;
		}

		if (dname_frag && buf[i] != '\"') {
			dname = dname + buf[i];
		}
		else if (!dname_frag && dname != "") {
			conf.push_back(dname);
			dname = "";
			count++;
		}

		if (buf[i] == '=') {
			dname_frag = true;
		}

	}
	conf.push_back(to_string(count));
	cout << count << " load the trajectory of the folder." << endl;

	cout << "Configuration file loading complete!" << endl;
	printf("\n");
	return conf;
}

//Load trajectory data(Trajectory[0]-[worker number]、Trajectory[][0] is file name、Trajectory[][1] is header、Trajectory[][2-] is x,y,unixtime)
vector<vector<string>> Trajectory(string result_dname, string tra_dir) {
	cout << tra_dir << " Loading trajectory file." << endl;
	WIN32_FIND_DATA win32fd;
	HANDLE hFind;
	vector<vector<string>> tra_all_data;

	string search_fname = ".\\" + tra_dir + "\\*.*";
	hFind = FindFirstFile(search_fname.c_str(), &win32fd);
	if (hFind == INVALID_HANDLE_VALUE) {
		cerr << tra_dir << " not found" << endl;
	}
	else {
		do {
			if (win32fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			}
			else {
				vector<string> tra_data;
				string sFileName(win32fd.cFileName);
				cout << sFileName << endl;
				tra_data.push_back(sFileName);
				ifstream ifs(".\\" + tra_dir + "\\" + win32fd.cFileName);
				string buf;
				while (ifs && getline(ifs, buf)) {
					tra_data.push_back(buf);
				}
				tra_all_data.push_back(tra_data);
			}
		} while (FindNextFile(hFind, &win32fd));
		FindClose(hFind);
	}

	string score_dname = ".\\" + tra_dir + result_dname;
	if (_mkdir(score_dname.c_str()) == 0) {
		cout << score_dname << " directory was created." << endl;
	}
	else {
		cerr << score_dname << " directory was not created." << endl;
	}

	cout << "Trajectory file loading complete!" << endl;
	printf("\n");
	return tra_all_data;
}

//Load ture_data
//True_data[0] is obstacle data、True_data[1-5] is all WMS data[][0](fname)[][1](header)[][2-](worker_no,shelf_no,front_x,front_y,unixtime)、True_data[6-28] is test WMS data[][0](fname)[][1](header)[][2-](unixtime,shelf_no,front_x,front_y)、True_data[29] is VDR compatible data(worker_name,ble_id,worker_no)、True_data[30] is sensor start to end time data(fname,start_time,end_time)、True_data[31] is forklift entry impossible area(x,y)
vector<vector<string>> True_data(string mat_dname, string all_obstacle_fname, string all_wms_fname, string test_wms_fname, string pdr2vdr_wms_fname, string sens_start_end_fname, string forklift_no_invasion_fname) {
	cout << "Loading true-data file." << endl;
	vector<vector<string>> all_data;

	ifstream ifs_all_obstacle(".\\" + mat_dname + "\\" + all_obstacle_fname);
	ifstream ifs_all_wms(".\\" + mat_dname + "\\" + all_wms_fname);
	ifstream ifs_test_wms(".\\" + mat_dname + "\\" + test_wms_fname);
	ifstream ifs_pdr2vdr_wms(".\\" + mat_dname + "\\" + pdr2vdr_wms_fname);
	ifstream ifs_sens_start_end(".\\" + mat_dname + "\\" + sens_start_end_fname);
	ifstream ifs_forklift_no_invasion(".\\" + mat_dname + "\\" + forklift_no_invasion_fname);
	string buf;
	vector<string> all_obstacle_data;
	while (ifs_all_obstacle && getline(ifs_all_obstacle, buf)) {
		all_obstacle_data.push_back(buf);
	}
	all_data.push_back(all_obstacle_data);

	while (ifs_all_wms && getline(ifs_all_wms, buf)) {
		vector<string> all_wms_data;
		ifstream ifs_all_wms_day(".\\" + mat_dname + "\\" + buf);
		string buf_day;
		all_wms_data.push_back(buf);
		while (ifs_all_wms_day && getline(ifs_all_wms_day, buf_day)) {
			all_wms_data.push_back(buf_day);
		}
		all_data.push_back(all_wms_data);
	}

	while (ifs_test_wms && getline(ifs_test_wms, buf)) {
		vector<string> test_wms_data;
		ifstream ifs_test_wms_day_worker(".\\" + mat_dname + "\\" + buf);
		string buf_day;
		test_wms_data.push_back(buf);
		while (ifs_test_wms_day_worker && getline(ifs_test_wms_day_worker, buf_day)) {
			test_wms_data.push_back(buf_day);
		}
		all_data.push_back(test_wms_data);
	}

	vector<string> pdr2vdr_wms_data;
	while (ifs_pdr2vdr_wms && getline(ifs_pdr2vdr_wms, buf)) {
		pdr2vdr_wms_data.push_back(buf);
	}
	all_data.push_back(pdr2vdr_wms_data);

	vector<string> sens_start_end_data;
	while (ifs_sens_start_end && getline(ifs_sens_start_end, buf)) {
		sens_start_end_data.push_back(buf);
	}
	all_data.push_back(sens_start_end_data);

	vector<string> forklift_no_invasion;
	while (ifs_forklift_no_invasion && getline(ifs_forklift_no_invasion, buf)) {
		forklift_no_invasion.push_back(buf);
	}
	all_data.push_back(forklift_no_invasion);

	cout << "True-data file loading complete!" << endl;
	printf("\n");
	return all_data;
}


#define ALL_WMS_NUMBER 5
#define TEST_WMS_NUMBER 28
#define PDR2VDR_WMS 29
#define SENS_START_END 30
#define FORKLIFT_NO_INVASION 31
//Distance error calculation([worker number][0]"xDR_No_date" [1-]true_x,true_y,true_time,tra_x,tra_y,tra_time,dif_time,error_m,BUP_T/F)
vector<vector<string>> EdEs_dif(string result_dname, vector<vector<string>> true_data, vector<vector<string>> tra_data, string tra_dir) {
	cout << "Calculating distance error." << endl;

	vector<string> forklift_no_leftdown_element = split(true_data[FORKLIFT_NO_INVASION][1], ',');//Forklift entry impossible area bottom left coordinates
	vector<string> forklift_no_rightup_element = split(true_data[FORKLIFT_NO_INVASION][2], ',');//Forklift entry impossible area upper right coordinates

	vector<vector<string>> all_true_wms;
	for (int n = 1; n <= ALL_WMS_NUMBER; n++) {
		for (int m = ALL_WMS_NUMBER + 1; m <= TEST_WMS_NUMBER; m++) {
			if (true_data[m][0].find(true_data[n][0]) != string::npos) {

				vector<string> true_wms;
				true_wms.push_back(true_data[m][0]);

				string pdr_or_vdr_test = true_data[m][0].substr(0, 3);
				string worker_no_test = true_data[m][0].substr(4, 2);
				string date_test = true_data[m][0].substr(11, 8);

				int start_time;
				int end_time;
				bool start_end_flag = FALSE;
				for (int r = 1; r < true_data[SENS_START_END].size(); r++) {
					vector<string> sens_element = split(true_data[SENS_START_END][r], ',');
					string pdr_or_vdr_sens = sens_element[0].substr(0, 3);
					string worker_no_sens = sens_element[0].substr(9, 2);
					string date_sens = sens_element[0].substr(12, 8);
					if (pdr_or_vdr_test == pdr_or_vdr_sens && worker_no_test == worker_no_sens && date_test == date_sens) {
						start_time = stod(sens_element[1]);
						end_time = stod(sens_element[2]);
						start_end_flag = TRUE;
						break;
					}
				}
				if (start_end_flag == FALSE) {
					cerr << "ERROR: Initialization failure of sensor data start and end time failed." << endl;
				}

				//VDR associates forklift_number with worker_number
				if (pdr_or_vdr_test == "VDR") {
					bool forklift_no_flag = FALSE;
					for (int s = 1; s < true_data[PDR2VDR_WMS].size(); s++) {
						vector<string> pdr2vdr_element = split(true_data[PDR2VDR_WMS][s], ',');
						if (worker_no_test == pdr2vdr_element[2] || worker_no_test == pdr2vdr_element[3]) {
							worker_no_test = pdr2vdr_element[1];
							forklift_no_flag = TRUE;
							break;
						}
					}
					if (forklift_no_flag == FALSE) {
						cerr << "ERROR: Failed to rename VDR number." << endl;
					}
				}

				for (int o = 2; o < true_data[n].size(); o++) {
					vector<string> all_wms_element = split(true_data[n][o], ',');

					if (worker_no_test == all_wms_element[0] && stoi(all_wms_element[4]) > start_time && stoi(all_wms_element[4]) < end_time) {

						if (pdr_or_vdr_test == "VDR" && stod(forklift_no_leftdown_element[0]) < stod(all_wms_element[2]) && stod(forklift_no_rightup_element[0]) > stod(all_wms_element[2]) && stod(forklift_no_leftdown_element[1]) < stod(all_wms_element[3]) && stod(forklift_no_rightup_element[1]) > stod(all_wms_element[3])) {
							//cout << "x: " << all_wms_element[2] << ", y: " << all_wms_element[3] << " FORKLIFT_NO_INVASION" << endl;
						}
						else {

							bool truewms_flag = TRUE;
							bool bup_flag = FALSE;
							int dif_time_min = 10000000;//huge number
							int corr_test_wms_time;
							int corr_test_wms_count;
							for (int p = 2; p < true_data[m].size(); p++) {
								vector<string> test_wms_element = split(true_data[m][p], ',');
								if (all_wms_element[4] == test_wms_element[0]) {
									truewms_flag = FALSE;
								}
								else {
									int dif_time = abs(stoi(all_wms_element[4]) - stoi(test_wms_element[0]));
									if (dif_time_min > dif_time) {
										dif_time_min = dif_time;
										corr_test_wms_time = stoi(test_wms_element[0]);
										corr_test_wms_count = p - 2;
									}
								}
							}
							if (truewms_flag == TRUE) {
								//Judgment BUP
								if (signbit(double(stoi(all_wms_element[4]) - corr_test_wms_time)) && corr_test_wms_count % 2 != 0) {
									bup_flag = TRUE;
								}
								else if (!signbit(double(stoi(all_wms_element[4]) - corr_test_wms_time)) && corr_test_wms_count % 2 == 0) { 
									bup_flag = TRUE;
								}
								string true_wms_str = all_wms_element[4] + "," + all_wms_element[2] + "," + all_wms_element[3] + "," + to_string(bup_flag);//unixtime,front_x,front_y,bup_T/F
								true_wms.push_back(true_wms_str);
							}

						}
					}
				}
				all_true_wms.push_back(true_wms);
			}
		}
	}

	vector<vector<string>> all_dif_data;
	for (int i = 0; i < all_true_wms.size(); i++) {
		cout << all_true_wms[i][0] << endl;
		cout << "              %";
		string wms_pdr_or_vdr = all_true_wms[i][0].substr(0, 3);
		string wms_worker_no = all_true_wms[i][0].substr(4, 2);
		string wms_date = all_true_wms[i][0].substr(11, 8);
		int tra_data_count;
		bool tra_data_flag = FALSE;
		for (int k = 0; k < tra_data.size(); k++) {
			string tra_pdr_or_vdr = tra_data[k][0].substr(0, 3);
			string tra_worker_no = tra_data[k][0].substr(11, 2);
			string tra_date = tra_data[k][0].substr(14, 8);

			if (wms_pdr_or_vdr == tra_pdr_or_vdr && wms_worker_no == tra_worker_no && wms_date == tra_date) {
				tra_data_count = k;
				tra_data_flag = TRUE;
				break;
			}
		}
		if (tra_data_flag == FALSE) {
			printf("\r");
			cout << "No trajectory corresponding to true-data." << endl;
		}
		else {
			vector<string> dif_data;
			dif_data.push_back(wms_pdr_or_vdr + "_" + wms_worker_no + "_" + wms_date);

			for (int j = 1; j < all_true_wms[i].size(); j++) {
				printf("\r");
				cout << 100.0 * (double(j) / double(all_true_wms[i].size()));
				vector<string> true_element = split(all_true_wms[i][j], ',');
				double dif_time_min = 10000000.0;//huge number
				string corr_tra_x;
				string corr_tra_y;
				string corr_tra_time;
				for (int l = 2; l < tra_data[tra_data_count].size(); l++) {
					vector<string> tra_element = split(tra_data[tra_data_count][l], ',');

					double dif_time = abs(stod(true_element[0]) - stod(tra_element[2]));
					if (dif_time_min > dif_time) {
						dif_time_min = dif_time;
						corr_tra_x = tra_element[0];
						corr_tra_y = tra_element[1];
						corr_tra_time = tra_element[2];
					}
				}
				double error_m = hypot(stod(true_element[1]) - stod(corr_tra_x), stod(true_element[2]) - stod(corr_tra_y));

				string dif_data_str = true_element[1] + "," + true_element[2] + "," + true_element[0] + "," + corr_tra_x + "," + corr_tra_y + "," + corr_tra_time + "," + to_string(dif_time_min) + "," + to_string(error_m) + "," + true_element[3];
				dif_data.push_back(dif_data_str);
			}

			all_dif_data.push_back(dif_data);

			printf("\r");
			cout << "finish!        " << endl;
		}
	}

	for (int i = 0; i < all_dif_data.size(); i++) {
		ofstream ofs(".\\" + tra_dir + result_dname + "\\EdEs_dif_" + all_dif_data[i][0] + ".csv");
		for (int j = 1; j < all_dif_data[i].size(); j++) {
			ofs << all_dif_data[i][j] << endl;
		}
	}

	cout << "Distance error calculating complete!" << endl;
	printf("\n");
	return all_dif_data;
}

#define MAX_RANGE 17952.25
//Ed(fname,score,error_median,score_not_bup,error_median_not_bup,score_bup,error_median_bup)
vector<string> Ed(string result_dname, vector<vector<string>> EdEs_dif, string tra_dir) {
	cout << "Evaluating E_d." << endl;
	vector<string> score_data;
	double median_ave = 0.0;
	double score_ave = 0.0;
	double median_not_bup_ave = 0.0;
	double score_not_bup_ave = 0.0;
	double median_bup_ave = 0.0;
	double score_bup_ave = 0.0;
	vector<double> median_median;
	vector<double> score_median;
	vector<double> median_not_bup_median;
	vector<double> score_not_bup_median;
	vector<double> median_bup_median;
	vector<double> score_bup_median;

	for (int i = 0; i < EdEs_dif.size(); i++) {
		vector<double> error_data;
		vector<double> error_data_not_bup;
		vector<double> error_data_bup;
		for (int j = 1; j < EdEs_dif[i].size(); j++) {
			vector<string> dif_element = split(EdEs_dif[i][j], ',');

			if (stod(dif_element[6]) < 5) {

				double error_m = stod(dif_element[7]);

				if (stoi(dif_element[8]) == 0) {
					error_data_not_bup.push_back(error_m);
				}
				else {
					error_data_bup.push_back(error_m);
				}

				error_data.push_back(error_m);

			}
		}

		double score;
		double median;
		if (error_data.size() == 0) {
			score = 0.0;
			median = MAX_RANGE;
		}
		else {
			sort(error_data.begin(), error_data.end());
			median = error_data[error_data.size() / 2];
			score = -(100.0 / 29.0) * median + (3000.0 / 29.0);//score
			if (score < 0.0) {
				score = 0.0;
			}
			else if (score > 100.0) {
				score = 100.0;
			}
		}

		double score_not_bup;
		double median_not_bup;
		if (error_data_not_bup.size() == 0) {
			score_not_bup = 0.0;
			median_not_bup = MAX_RANGE;
		}
		else {
			sort(error_data_not_bup.begin(), error_data_not_bup.end());
			median_not_bup = error_data_not_bup[error_data_not_bup.size() / 2];
			score_not_bup = -(100.0 / 29.0) * median_not_bup + (3000.0 / 29.0);//Not_BUP score
			if (score_not_bup < 0.0) {
				score_not_bup = 0.0;
			}
			else if (score_not_bup > 100.0) {
				score_not_bup = 100.0;
			}
		}

		double score_bup;
		double median_bup;
		if (error_data_bup.size() == 0) {
			score_bup = 0.0;
			median_bup = MAX_RANGE;
		}
		else {
			sort(error_data_bup.begin(), error_data_bup.end());
			median_bup = error_data_bup[error_data_bup.size() / 2];
			score_bup = -(100.0 / 29.0) * median_bup + (3000.0 / 29.0);//BUP score
			if (score_bup < 0.0) {
				score_bup = 0.0;
			}
			else if (score_bup > 100.0) {
				score_bup = 100.0;
			}
		}

		string score_str = EdEs_dif[i][0] + "," + to_string(score) + "," + to_string(median) + "," + to_string(score_not_bup) + "," + to_string(median_not_bup) + "," + to_string(score_bup) + "," + to_string(median_bup);

		median_ave = median_ave + median;
		score_ave = score_ave + score;
		median_not_bup_ave = median_not_bup_ave + median_not_bup;
		score_not_bup_ave = score_not_bup_ave + score_not_bup;
		median_bup_ave = median_bup_ave + median_bup;
		score_bup_ave = score_bup_ave + score_bup;
		score_data.push_back(score_str);

		median_median.push_back(median);
		score_median.push_back(score);
		median_not_bup_median.push_back(median_not_bup);
		score_not_bup_median.push_back(score_not_bup);
		median_bup_median.push_back(median_bup);
		score_bup_median.push_back(score_bup);

		error_data.clear();
		error_data.shrink_to_fit();
	}
	median_ave = median_ave / EdEs_dif.size();
	score_ave = score_ave / EdEs_dif.size();
	median_not_bup_ave = median_not_bup_ave / EdEs_dif.size();
	score_not_bup_ave = score_not_bup_ave / EdEs_dif.size();
	median_bup_ave = median_bup_ave / EdEs_dif.size();
	score_bup_ave = score_bup_ave / EdEs_dif.size();
	string score_str = "score_median_ave," + to_string(score_ave) + "," + to_string(median_ave) + "," + to_string(score_not_bup_ave) + "," + to_string(median_not_bup_ave) + "," + to_string(score_bup_ave) + "," + to_string(median_bup_ave);
	score_data.push_back(score_str);

	sort(median_median.begin(), median_median.end());
	double err_median = median_median[median_median.size() / 2];

	sort(score_median.begin(), score_median.end());
	double sco_median = score_median[score_median.size() / 2];

	sort(median_not_bup_median.begin(), median_not_bup_median.end());
	double err_not_bup_median = median_not_bup_median[median_not_bup_median.size() / 2];

	sort(score_not_bup_median.begin(), score_not_bup_median.end());
	double sco_not_bup_median = score_not_bup_median[score_not_bup_median.size() / 2];

	sort(median_bup_median.begin(), median_bup_median.end());
	double err_bup_median = median_bup_median[median_bup_median.size() / 2];

	sort(score_bup_median.begin(), score_bup_median.end());
	double sco_bup_median = score_bup_median[score_bup_median.size() / 2];

	score_str = "score_median_median," + to_string(sco_median) + "," + to_string(err_median) + "," + to_string(sco_not_bup_median) + "," + to_string(err_not_bup_median) + "," + to_string(sco_bup_median) + "," + to_string(err_bup_median);
	score_data.push_back(score_str);

	ofstream ofs(".\\" + tra_dir + result_dname + "\\Ed_score.csv");
	ofs << "name,score_all_section,error_all_section,score_not_BUP_section,error_not_BUP_section,score_BUP_section,error_BUP_section" << endl;
	for (int i = 0; i < score_data.size(); i++) {
		ofs << score_data[i] << endl;
	}

	cout << "E_d evaluating complete!" << endl;
	printf("\n");
	return score_data;
}


//Es(fname,score,median_m_s,score_not_bup,median_m_s_not_bup,score_bup,median_m_s_bup)
vector<string> Es(string result_dname, vector<vector<string>> EdEs_dif, vector<vector<string>> true_data, string tra_dir) {
	cout << "Evaluating E_s." << endl;
	vector<string> score_data;
	double median_ave = 0.0;
	double score_ave = 0.0;
	double median_not_bup_ave = 0.0;
	double score_not_bup_ave = 0.0;
	double median_bup_ave = 0.0;
	double score_bup_ave = 0.0;
	vector<double> median_median;
	vector<double> score_median;
	vector<double> median_not_bup_median;
	vector<double> score_not_bup_median;
	vector<double> median_bup_median;
	vector<double> score_bup_median;

	for (int i = 0; i < EdEs_dif.size(); i++) {
		vector<double> error_m_s_data;
		vector<double> error_m_s_data_not_bup;
		vector<double> error_m_s_data_bup;

		cout << EdEs_dif[i][0] << endl;
		string dif_pdr_or_vdr = EdEs_dif[i][0].substr(0, 3);
		string dif_worker_no = EdEs_dif[i][0].substr(4, 2);
		string dif_date = EdEs_dif[i][0].substr(7, 8);

		ofstream ofs_m_s(".\\" + tra_dir + result_dname + "\\" + EdEs_dif[i][0].substr(0, 24) + "_error_m_s.csv");
		ofs_m_s << "error_m_s,error_m,dif_time_min,BUP_T/F,unixtime" << endl;
		for (int j = 1; j < EdEs_dif[i].size(); j++) {
			vector<string> dif_element = split(EdEs_dif[i][j], ',');
			double error_m = stod(dif_element[7]);
			double tra_time = stod(dif_element[5]);
			double dif_time_min = 10000000.0;//huge number

			for (int k = ALL_WMS_NUMBER + 1; k <= TEST_WMS_NUMBER; k++) {
				string test_pdr_or_vdr = true_data[k][0].substr(0, 3);
				string test_worker_no = true_data[k][0].substr(4, 2);
				string test_date = true_data[k][0].substr(11, 8);
				if (dif_pdr_or_vdr == test_pdr_or_vdr && dif_worker_no == test_worker_no && dif_date == test_date) {
					for (int l = 2; l < true_data[k].size(); l++) {
						vector<string> test_wms_element = split(true_data[k][l], ',');
						int test_wms_time = stoi(test_wms_element[0]);

						double dif_time = abs(tra_time - test_wms_time);
						if (dif_time_min > dif_time) {
							dif_time_min = dif_time;
						}
					}

					if (dif_time_min <= 1800.0) {
						double error_m_s = error_m / dif_time_min;

						if (stoi(dif_element[8]) == 0) {
							error_m_s_data_not_bup.push_back(error_m_s);
						}
						else {
							error_m_s_data_bup.push_back(error_m_s);
						}

						error_m_s_data.push_back(error_m_s);
						ofs_m_s << to_string(error_m_s) + "," + to_string(error_m) + "," + to_string(dif_time_min) + "," + dif_element[8] + "," + dif_element[5] << endl;
					}

					break;
				}
			}
		}
		if ((EdEs_dif[i].size() - 1) != error_m_s_data.size()) {
			cout << (EdEs_dif[i].size() - 1) << ", " << error_m_s_data.size() << endl;
		}

		double median;
		if (error_m_s_data.size() == 0) {
			median = MAX_RANGE;
		}
		else {
			sort(error_m_s_data.begin(), error_m_s_data.end());
			median = error_m_s_data[error_m_s_data.size() / 2];
		}

		double median_not_bup;
		if (error_m_s_data_not_bup.size() == 0) {
			median_not_bup = MAX_RANGE;
		}
		else {
			sort(error_m_s_data_not_bup.begin(), error_m_s_data_not_bup.end());
			median_not_bup = error_m_s_data_not_bup[error_m_s_data_not_bup.size() / 2];
		}

		double median_bup;
		if (error_m_s_data_bup.size() == 0) {
			median_bup = MAX_RANGE;
		}
		else {
			sort(error_m_s_data_bup.begin(), error_m_s_data_bup.end());
			median_bup = error_m_s_data_bup[error_m_s_data_bup.size() / 2];
		}

		double score = -(100.0 / 1.95) * median + (200.0 / 1.95);//score
		if (score < 0.0) {
			score = 0.0;
		}
		else if (score > 100.0) {
			score = 100.0;
		}

		double score_not_bup = -(100.0 / 1.95) * median_not_bup + (200.0 / 1.95);//Not_BUP score
		if (score_not_bup < 0.0) {
			score_not_bup = 0.0;
		}
		else if (score_not_bup > 100.0) {
			score_not_bup = 100.0;
		}

		double score_bup = -(100.0 / 1.95) * median_bup + (200.0 / 1.95);//BUP score
		if (score_bup < 0.0) {
			score_bup = 0.0;
		}
		else if (score_bup > 100.0) {
			score_bup = 100.0;
		}

		string score_str = EdEs_dif[i][0] + "," + to_string(score) + "," + to_string(median) + "," + to_string(score_not_bup) + "," + to_string(median_not_bup) + "," + to_string(score_bup) + "," + to_string(median_bup);
		score_data.push_back(score_str);
		score_ave = score_ave + score;
		median_ave = median_ave + median;
		score_not_bup_ave = score_not_bup_ave + score_not_bup;
		median_not_bup_ave = median_not_bup_ave + median_not_bup;
		score_bup_ave = score_bup_ave + score_bup;
		median_bup_ave = median_bup_ave + median_bup;
		median_median.push_back(median);
		score_median.push_back(score);
		median_not_bup_median.push_back(median_not_bup);
		score_not_bup_median.push_back(score_not_bup);
		median_bup_median.push_back(median_bup);
		score_bup_median.push_back(score_bup);

	}
	score_ave = score_ave / EdEs_dif.size();
	median_ave = median_ave / EdEs_dif.size();
	score_not_bup_ave = score_not_bup_ave / EdEs_dif.size();
	median_not_bup_ave = median_not_bup_ave / EdEs_dif.size();
	score_bup_ave = score_bup_ave / EdEs_dif.size();
	median_bup_ave = median_bup_ave / EdEs_dif.size();
	string score_str = "score_median_ave," + to_string(score_ave) + "," + to_string(median_ave) + "," + to_string(score_not_bup_ave) + "," + to_string(median_not_bup_ave) + "," + to_string(score_bup_ave) + "," + to_string(median_bup_ave);
	score_data.push_back(score_str);

	sort(median_median.begin(), median_median.end());
	double err_median = median_median[median_median.size() / 2];

	sort(score_median.begin(), score_median.end());
	double sco_median = score_median[score_median.size() / 2];

	sort(median_not_bup_median.begin(), median_not_bup_median.end());
	double err_not_bup_median = median_not_bup_median[median_not_bup_median.size() / 2];

	sort(score_not_bup_median.begin(), score_not_bup_median.end());
	double sco_not_bup_median = score_not_bup_median[score_not_bup_median.size() / 2];

	sort(median_bup_median.begin(), median_bup_median.end());
	double err_bup_median = median_bup_median[median_bup_median.size() / 2];

	sort(score_bup_median.begin(), score_bup_median.end());
	double sco_bup_median = score_bup_median[score_bup_median.size() / 2];

	score_str = "score_median_median," + to_string(sco_median) + "," + to_string(err_median) + "," + to_string(sco_not_bup_median) + "," + to_string(err_not_bup_median) + "," + to_string(sco_bup_median) + "," + to_string(err_bup_median);
	score_data.push_back(score_str);

	ofstream ofs(".\\" + tra_dir + result_dname + "\\Es_score.csv");
	ofs << "name,score_all_section,error_all_section,score_not_BUP_section,error_not_BUP_section,score_BUP_section,error_BUP_section" << endl;
	for (int i = 0; i < score_data.size(); i++) {
		ofs << score_data[i] << endl;
	}

	cout << "E_s evaluating complete!" << endl;
	printf("\n");
	return score_data;
}


//Ep(fname,score)
#define ALLOWABLE_MOVEMENT_ERROR 1 //Maximum allowable movement error at picking[m]
#define TARGET_TIME 3 //Picking time[sec]
vector<string> Ep(string result_dname, vector<vector<string>> tra_data, vector<vector<string>> EdEs_dif, string tra_dir) {
	cout << "Evaluating E_p." << endl;
	vector<string> score_data;
	double score_ave = 0;
	vector<double> score_median;

	for (int i = 0; i < EdEs_dif.size(); i++) {
		cout << EdEs_dif[i][0] << endl;
		cout << "              %";
		int  count = 0;
		for (int j = 1; j < EdEs_dif[i].size(); j++) {
			printf("\r");
			cout << 100.0 * (double(j) / double(EdEs_dif[i].size()));
			vector<string> dif_element = split(EdEs_dif[i][j], ',');
			double target_time1 = stod(dif_element[5]) - (TARGET_TIME / 2);
			double target_time2 = stod(dif_element[5]) + (TARGET_TIME / 2);
			double x_t1 = 0;
			double y_t1 = 0;
			double dif_m = 0;
			bool frag = false;
			for (int k = 2; k < tra_data[i].size() - 1; k++) {
				vector<string> tra_element = split(tra_data[i][k], ',');
				if (target_time1 <= stod(tra_element[2]) && stod(tra_element[2]) <= target_time2) {
					frag = true;
					if (x_t1 == 0 && y_t1 == 0) {
						x_t1 = stod(tra_element[0]);
						y_t1 = stod(tra_element[1]);
					}
					else {
						dif_m = dif_m + hypot(x_t1 - stod(tra_element[0]), y_t1 - stod(tra_element[1]));
						x_t1 = stod(tra_element[0]);
						y_t1 = stod(tra_element[1]);
					}
				}
				else {
					if (frag) {
						break;
					}
				}
			}
			if (dif_m <= ALLOWABLE_MOVEMENT_ERROR) {
				count++;
			}
		}
		double score = 100.0 * count / EdEs_dif[i].size();
		string score_str = EdEs_dif[i][0] + "," + to_string(score);
		score_data.push_back(score_str);
		score_ave = score_ave + score;
		score_median.push_back(score);

		printf("\r");
		cout << "finish!        " << endl;

	}
	score_ave = score_ave / EdEs_dif.size();
	string score_str = "score_ave," + to_string(score_ave);
	score_data.push_back(score_str);

	sort(score_median.begin(), score_median.end());
	double median = score_median[score_median.size() / 2];
	score_str = "score_median," + to_string(median);
	score_data.push_back(score_str);


	ofstream ofs(".\\" + tra_dir + result_dname + "\\Ep_score.csv");
	ofs << "name,score" << endl;
	for (int i = 0; i < score_data.size(); i++) {
		ofs << score_data[i] << endl;
	}

	cout << "E_p evaluating complete!" << endl;
	printf("\n");
	return score_data;
}


//Ev(fname,score)
#define PEDESTRIAN_VELOCITY 1.5 //Maximum allowable walking speed[m/sec]
#define VEHICLE_VELOCITY 4.5 //Maximum allowable forklift speed[m/sec]
vector<string> Ev(string result_dname, vector<vector<string>> tra_data, vector<vector<string>> EdEs_dif, string tra_dir) {
	cout << "Evaluating E_v." << endl;
	vector<string> score_data;
	double score_ave = 0;
	vector<double> score_median;

	for (int i = 0; i < EdEs_dif.size(); i++) {
		cout << EdEs_dif[i][0] << endl;
		cout << "              %";
		string dif_pdr_or_vdr = EdEs_dif[i][0].substr(0, 3);
		string dif_worker_no = EdEs_dif[i][0].substr(4, 2);
		string dif_date = EdEs_dif[i][0].substr(7, 8);

		for (int j = 0; j < tra_data.size(); j++) {
			string tra_pdr_or_vdr = tra_data[j][0].substr(0, 3);
			string tra_worker_no = tra_data[j][0].substr(11, 2);
			string tra_date = tra_data[j][0].substr(14, 8);

			if (dif_pdr_or_vdr == tra_pdr_or_vdr && dif_worker_no == tra_worker_no && dif_date == tra_date) {
				int all_count = 0;
				int count = 0;
				double dif_m_add = 0;

				double velocity_max;
				if (tra_data[j][0].find("PDR") != string::npos) {
					velocity_max = PEDESTRIAN_VELOCITY;
				}
				else {
					velocity_max = VEHICLE_VELOCITY;
				}
				for (int k = 2; k < tra_data[j].size() - 1; k++) {
					printf("\r");
					cout << 100.0 * (double(k) / double(tra_data[j].size()));
					vector<string> tra_element = split(tra_data[j][k], ',');
					vector<string> tra_t1_element = split(tra_data[j][k + 1], ',');
					double dif_m = hypot(stod(tra_element[0]) - stod(tra_t1_element[0]), stod(tra_element[1]) - stod(tra_t1_element[1]));
					double dif_sec = abs(stod(tra_t1_element[2]) - stod(tra_element[2]));
					dif_m_add = dif_m_add + dif_m;
					if (dif_sec != 0) {
						all_count++;
						double velocity = dif_m_add / dif_sec;
						dif_m_add = 0;

						if (velocity > velocity_max) {
							count++;
						}
					}
				}
				double score = 100.0 * (all_count - count) / all_count;
				string score_str = EdEs_dif[i][0] + "," + to_string(score);
				score_data.push_back(score_str);
				score_ave = score_ave + score;
				score_median.push_back(score);

				break;
			}

		}
		printf("\r");
		cout << "finish!        " << endl;
	}
	if (EdEs_dif.size() != score_data.size()) {
		printf("\n");
		cerr << EdEs_dif.size() << ", " << score_data.size() << endl;
		printf("\n");
	}

	score_ave = score_ave / EdEs_dif.size();
	string score_str = "score_ave," + to_string(score_ave);
	score_data.push_back(score_str);

	sort(score_median.begin(), score_median.end());
	double median = score_median[score_median.size() / 2];
	score_str = "score_median," + to_string(median);
	score_data.push_back(score_str);


	ofstream ofs(".\\" + tra_dir + result_dname + "\\Ev_score.csv");
	ofs << "name,score" << endl;
	for (int i = 0; i < score_data.size(); i++) {
		ofs << score_data[i] << endl;
	}

	cout << "E_v evaluating complete!" << endl;
	printf("\n");
	return score_data;
}


//Eo(fname,score)
#define RANGE_CONSIDERED 0.14
vector<string> Eo(string result_dname, vector<string> all_obstacle_data, vector<vector<string>> tra_data, vector<vector<string>> EdEs_dif, string tra_dir) {
	cout << "Evaluating E_o." << endl;
	vector<string> score_data;
	double score_ave = 0;
	vector<double> score_median;

	for (int i = 0; i < EdEs_dif.size(); i++) {
		cout << EdEs_dif[i][0] << endl;
		cout << "              %";
		string dif_pdr_or_vdr = EdEs_dif[i][0].substr(0, 3);
		string dif_worker_no = EdEs_dif[i][0].substr(4, 2);
		string dif_date = EdEs_dif[i][0].substr(7, 8);

		for (int j = 0; j < tra_data.size(); j++) {
			string tra_pdr_or_vdr = tra_data[j][0].substr(0, 3);
			string tra_worker_no = tra_data[j][0].substr(11, 2);
			string tra_date = tra_data[j][0].substr(14, 8);

			if (dif_pdr_or_vdr == tra_pdr_or_vdr && dif_worker_no == tra_worker_no && dif_date == tra_date) {

				int count_size = tra_data[j].size() - 2;
				int count = 0;
				for (int k = 2; k < tra_data[j].size(); k++) {
					printf("\r");
					cout << 100.0 * (double(k) / double(tra_data[j].size()));
					vector<string> tra_element = split(tra_data[j][k], ',');
					for (int l = 2; l < all_obstacle_data.size(); l++) {
						vector<string> obs_element = split(all_obstacle_data[l], ',');
						if (stod(tra_element[0]) >= stod(obs_element[0]) - (stod(obs_element[2]) / 2) + RANGE_CONSIDERED && stod(tra_element[0]) <= stod(obs_element[0]) + (stod(obs_element[2]) / 2) - RANGE_CONSIDERED
							&& stod(tra_element[1]) >= stod(obs_element[1]) - (stod(obs_element[3]) / 2) + RANGE_CONSIDERED && stod(tra_element[1]) <= stod(obs_element[1]) + (stod(obs_element[3]) / 2) - RANGE_CONSIDERED) {
							count++;
							break;
						}
					}
				}
				count = count_size - (count * 10);

				double score = 100.0 * count / (tra_data[j].size() - 2.0);
				string score_str = EdEs_dif[i][0] + "," + to_string(score);
				score_data.push_back(score_str);
				score_ave = score_ave + score;
				score_median.push_back(score);

				break;
			}

		}
		printf("\r");
		cout << "finish!        " << endl;

	}
	if (EdEs_dif.size() != score_data.size()) {
		printf("\n");
		cerr << EdEs_dif.size() << ", " << score_data.size() << endl;
		printf("\n");
	}

	score_ave = score_ave / EdEs_dif.size();
	string score_str = "score_ave," + to_string(score_ave);
	score_data.push_back(score_str);

	sort(score_median.begin(), score_median.end());
	double median = score_median[score_median.size() / 2];
	score_str = "score_median," + to_string(median);
	score_data.push_back(score_str);


	ofstream ofs(".\\" + tra_dir + result_dname + "\\Eo_score.csv");
	ofs << "name,score" << endl;
	for (int i = 0; i < score_data.size(); i++) {
		ofs << score_data[i] << endl;
	}

	printf("\r");
	cout << "E_o evaluating complete!" << endl;
	printf("\n");
	return score_data;
}


//Ef(id,score,tra_ratio)
vector<string> Ef(string result_dname, vector<vector<string>> tra_data, vector<vector<string>> EdEs_dif, string tra_dir, vector<vector<string>> true_data) {
	cout << "Evaluating E_f." << endl;
	vector<string> score_data;
	double score_ave = 0;
	vector<double> score_median;

	for (int i = 0; i < EdEs_dif.size(); i++) {
		cout << EdEs_dif[i][0] << endl;
		cout << "              %";
		string dif_pdr_or_vdr = EdEs_dif[i][0].substr(0, 3);
		string dif_worker_no = EdEs_dif[i][0].substr(4, 2);
		string dif_date = EdEs_dif[i][0].substr(7, 8);

		for (int j = 0; j < tra_data.size(); j++) {
			string tra_pdr_or_vdr = tra_data[j][0].substr(0, 3);
			string tra_worker_no = tra_data[j][0].substr(11, 2);
			string tra_date = tra_data[j][0].substr(14, 8);

			if (dif_pdr_or_vdr == tra_pdr_or_vdr && dif_worker_no == tra_worker_no && dif_date == tra_date) {

				double sens_start = 0;
				double sens_end = 0;
				for (int l = 1; l < true_data[SENS_START_END].size(); l++) {
					vector<string> sens_element = split(true_data[SENS_START_END][l], ',');
					string sens_pdr_or_vdr = sens_element[0].substr(0, 3);
					string sens_worker_no = sens_element[0].substr(9, 2);
					string sens_date = sens_element[0].substr(12, 8);
					if (dif_pdr_or_vdr == sens_pdr_or_vdr && dif_worker_no == sens_worker_no && dif_date == sens_date) {
						sens_start = stod(sens_element[1]);
						sens_end = stod(sens_element[2]);
					}
				}

				int count = tra_data[j].size() - 3;
				double tra_start, tra_end;
				for (int k = 2; k < tra_data[j].size() - 1; k++) {
					printf("\r");
					cout << 100.0 * (double(k) / double(tra_data[j].size()));
					vector<string> tra_element = split(tra_data[j][k], ',');
					vector<string> tra_t1_element = split(tra_data[j][k + 1], ',');

					//Penalty is imposed when the trajectory is shorter than the sensor measurement time
					if (k == 2) {
						tra_start = stod(tra_element[2]);
					}
					else if (k == tra_data[j].size() - 2) {
						tra_end = stod(tra_t1_element[2]);
					}


					double dif_sec = abs(stod(tra_t1_element[2]) - stod(tra_element[2]));
					if (dif_sec > 2) {
						count--;
					}
				}
				
				double score;
				double ratio;
				if (count < 0) {
					score = 0;
					ratio = 0;
				}
				else {
					score = 100.0 * count / (tra_data[j].size() - 3.0);

					ratio = abs(tra_end - tra_start) / abs(sens_end - sens_start);
					score = score * ratio;
				}


				string score_str = EdEs_dif[i][0] + "," + to_string(score) + "," + to_string(ratio);
				score_data.push_back(score_str);
				score_ave = score_ave + score;
				score_median.push_back(score);

				break;
			}
		}
		printf("\r");
		cout << "finish!        " << endl;

	}
	if (EdEs_dif.size() != score_data.size()) {
		printf("\n");
		cout << EdEs_dif.size() << "!=" << score_data.size() << endl;
		printf("\n");
	}

	score_ave = score_ave / EdEs_dif.size();
	string score_str = "score_ave," + to_string(score_ave);
	score_data.push_back(score_str);

	sort(score_median.begin(), score_median.end());
	double median = score_median[score_median.size() / 2];
	score_str = "score_median," + to_string(median);
	score_data.push_back(score_str);


	ofstream ofs(".\\" + tra_dir + result_dname + "\\Ef_score.csv");
	ofs << "name,score,overall_ratio" << endl;
	for (int i = 0; i < score_data.size(); i++) {
		ofs << score_data[i] << endl;
	}

	cout << "E_f evaluating complete!" << endl;
	printf("\n");
	return score_data;
}


//C.E.(id,score)
vector<string> CE(string result_dname, vector<string> Ed, vector<string> Es, vector<string> Ep, vector<string> Ev, vector<string> Eo, vector<string> Ef, string tra_dir) {
	cout << "Evaluating C.E." << endl;
	vector<string> score_data;
	double score_ave = 0;
	double score_ave_not_bup = 0;
	double score_ave_bup = 0;
	vector<double> score_median;
	vector<double> score_median_not_bup;
	vector<double> score_median_bup;

	for (int i = 0; i < Ed.size() - 1; i++) {
		vector<string> Ed_element = split(Ed[i], ',');
		vector<string> Es_element = split(Es[i], ',');
		vector<string> Ep_element = split(Ep[i], ',');
		vector<string> Ev_element = split(Ev[i], ',');
		vector<string> Eo_element = split(Eo[i], ',');
		vector<string> Ef_element = split(Ef[i], ',');

		double score = (0.3*stod(Ed_element[1])) + (0.3*stod(Es_element[1])) + (0.05*stod(Ep_element[1])) + (0.1*stod(Ev_element[1])) + (0.15*stod(Eo_element[1])) + (0.1*stod(Ef_element[1]));
		double score_not_bup = (0.3*stod(Ed_element[3])) + (0.3*stod(Es_element[3])) + (0.05*stod(Ep_element[1])) + (0.1*stod(Ev_element[1])) + (0.15*stod(Eo_element[1])) + (0.1*stod(Ef_element[1]));
		double score_bup = (0.3*stod(Ed_element[5])) + (0.3*stod(Es_element[5])) + (0.05*stod(Ep_element[1])) + (0.1*stod(Ev_element[1])) + (0.15*stod(Eo_element[1])) + (0.1*stod(Ef_element[1]));
		string score_str = Ed_element[0] + "," + to_string(score) + "," + to_string(score_not_bup) + "," + to_string(score_bup);
		score_data.push_back(score_str);
		score_ave = score_ave + score;
		score_ave_not_bup = score_ave_not_bup + score_not_bup;
		score_ave_bup = score_ave_bup + score_bup;
		score_median.push_back(score);
		score_median_not_bup.push_back(score_not_bup);
		score_median_bup.push_back(score_bup);
	}
	score_ave = score_ave / (Ed.size() - 1);
	score_ave_not_bup = score_ave_not_bup / (Ed.size() - 1);
	score_ave_bup = score_ave_bup / (Ed.size() - 1);
	string score_str = "score_ave," + to_string(score_ave) + "," + to_string(score_ave_not_bup) + "," + to_string(score_ave_bup);
	score_data.push_back(score_str);

	sort(score_median.begin(), score_median.end());
	double median = score_median[score_median.size() / 2];
	sort(score_median_not_bup.begin(), score_median_not_bup.end());
	double median_not_bup = score_median_not_bup[score_median_not_bup.size() / 2];
	sort(score_median_bup.begin(), score_median_bup.end());
	double median_bup = score_median_bup[score_median_bup.size() / 2];

	score_str = "score_median," + to_string(median) + "," + to_string(median_not_bup) + "," + to_string(median_bup);
	score_data.push_back(score_str);


	ofstream ofs(".\\" + tra_dir + result_dname + "\\CE_score.csv");
	ofs << "name,score_all_section,score_not_BUP_section,score_BUP_section" << endl;
	for (int i = 0; i < score_data.size(); i++) {
		ofs << score_data[i] << endl;
	}

	cout << "C.E. evaluating complete!" << endl;
	printf("\n");
	return score_data;
}


#define ALL_OBSTACLE 0
int main(int argc, char *argv[]) {
	time_t now = time(nullptr);
	string result_dname = "_score_" + to_string(now);

	vector<string> conf = Config();
	int team_count = stoi(conf[conf.size() - 1]);

	for (int i = 0; i < team_count; i++) {
		cout << conf[TRA_DNAME1 + i] << endl;
		//load_data
		vector<vector<string>> tra_data = Trajectory(result_dname, conf[TRA_DNAME1 + i]);
		vector<vector<string>> true_data = True_data(conf[MATERIAL_DNAME], conf[OBSTACLE_FNAME], conf[ALL_WMS_FNAME], conf[TEST_WMS_FNAME], conf[PDR2VDR_FNAME], conf[SENS_START_END_FNAME], conf[FORKLIFT_NO_INVASION_FNAME]);

		//cal_score
		vector<vector<string>> dif = EdEs_dif(result_dname, true_data, tra_data, conf[TRA_DNAME1 + i]);
		vector<string> Ed_score = Ed(result_dname, dif, conf[TRA_DNAME1 + i]);
		vector<string> Es_score = Es(result_dname, dif, true_data, conf[TRA_DNAME1 + i]);
		vector<string> Ep_score = Ep(result_dname, tra_data, dif, conf[TRA_DNAME1 + i]);
		vector<string> Ev_score = Ev(result_dname, tra_data, dif, conf[TRA_DNAME1 + i]);
		vector<string> Ef_score = Ef(result_dname, tra_data, dif, conf[TRA_DNAME1 + i], true_data);
		vector<string> Eo_score = Eo(result_dname, true_data[ALL_OBSTACLE], tra_data, dif, conf[TRA_DNAME1 + i]);
		vector<string> CE_score = CE(result_dname, Ed_score, Es_score, Ep_score, Ev_score, Eo_score, Ef_score, conf[TRA_DNAME1 + i]);
	}

}
