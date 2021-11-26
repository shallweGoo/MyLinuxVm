#include "web_server_config.h"
#include <iostream>

WebServerConfig::WebServerConfig() {
    m_port = 3333; //default web server port

    m_trig_mode = List_Conn_LT_LT; // 4 options to chose trigmode -> listenfd: LT + connfd : LT 

    m_lisenfd_trig_mode = LevelTriggerMode; 

    m_conn_trig_mode = LevelTriggerMode; 

    m_actor_model = ReactorMode;    

    m_linger = LingerDisable;   // elegent close flag switch, default close this opt

    m_thread_num = 8;

    //sql
    m_sql_conn_num = 8;

    m_sql_port = 3306;

    m_sql_username = "shallwe";

    m_sql_password = "Gxw960516!";      //loggin passwd

    m_sql_databasename = "sw_webserver";   // database name

    m_sql_tablename = "user";            // table name

    //log
    m_log_mode = LogAsyncMode; //sync mode

    m_log_status = LogEnable; // open it

    m_log_name = "./Log";

    //http
    m_hostname = "localhost";
};


void WebServerConfig::Parse(int argc, char* argv[]) {
    int opt;
    const char* str = "p:l:m:s:t:c:o:a:";
    while ((opt = getopt(argc, argv, str)) != -1) {
        switch (opt)
        {
        case 'p':
        {
            m_port = atoi(optarg);
            break;
        }
        case 'l':
        {
            m_log_status = atoi(optarg);
            break;
        }
        case 'm':
        {
            m_trig_mode = atoi(optarg);
            break;
        }
        case 's':
        {
            m_sql_conn_num = atoi(optarg);
            break;
        }
        case 't':
        {
            m_thread_num = atoi(optarg);
            break;
        }
        case 'o':
        {
            m_log_mode = atoi(optarg);
            break;
        }
        case 'a':
        {
            m_actor_model = atoi(optarg);
            break;
        }
        default:
            break;
        }
    }
}

void WebServerConfig::Show() const{
    using namespace std;
    cout << "***********************************" << endl;
    cout << "Show Settings:" << endl;
    cout << "***********************************" << endl;
    cout << "Webserver Settings:" << endl;
    cout << "web port : " << m_port << endl;
    cout << "web socker trigger mode : " << m_trig_mode << endl;
    if (m_actor_model == ReactorMode) {
        cout << "web server model : ReactorMode" << endl;
    } else {
        cout << "web server model : ProactorMode" << endl;
    }
    cout << "web server thread num : " << m_thread_num << endl;
    cout << "host : " << m_hostname << endl;
    cout << "***********************************" << endl;
    cout << "Mysql Settings:" << endl;
    cout << "port : " << m_sql_port << endl;
    cout << "connection num : " << m_sql_conn_num << endl;
    cout << "user : " << m_sql_username << endl;
    cout << "database : " << m_sql_databasename << endl;
    cout << "table : " << m_sql_tablename << endl;

    cout << "***********************************" << endl;
    cout << "Log Settings：" << endl;
    cout << "mode : " << m_log_mode << endl;
    cout << "log name : " << m_log_name << endl;
    cout << "***********************************" << endl;
}