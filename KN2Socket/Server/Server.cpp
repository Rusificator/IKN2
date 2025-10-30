#include <iostream>  
#include <winsock2.h> 
#include <windows.h> 
#include <string> 

#pragma comment (lib, "Ws2_32.lib")  
#define SRV_PORT 1234  
#define BUF_SIZE 1024

using namespace std;

struct Employee {
    char name[20];
    int projects_completed;
    int overtime_hours;
    int efficiency;
    int initiatives;
};

string calculateBonus(const Employee& emp) {
    int score = emp.projects_completed + emp.overtime_hours +
        emp.efficiency + emp.initiatives;

    if (score < 10) return "NO_BONUS";
    else if (score >= 10 && score < 20) return "STANDARD_BONUS";
    else if (score >= 20 && score < 30) return "MEDIUM_BONUS";
    else return "HIGH_BONUS";
}

void printNetworkInfo() {
    cout << "=== ИНФОРМАЦИЯ О СЕРВЕРЕ ===" << endl;

    // Получаем имя компьютера
    char computerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = sizeof(computerName);
    if (GetComputerNameA(computerName, &size)) {
        cout << "Имя компьютера: " << computerName << endl;
    }

    cout << "Порт сервера: " << SRV_PORT << endl;
    cout << "Сервер ожидает подключения..." << endl;
    cout << "Клиенты могут подключиться используя:" << endl;
    cout << "1. IP-адрес этого компьютера" << endl;
    cout << "2. Имя компьютера в сети" << endl;
    cout << "3. localhost (если клиент на этом же компьютере)" << endl;
    cout << "=================================" << endl;
}

int main() {
    setlocale(LC_ALL, "rus");
    cout << "\t Сервер расчета премий\n";
    for (int i = 0; i < 30; i++)
        cout << "-";
    cout << endl;

    char buff[1024];
    if (WSAStartup(0x0202, (WSADATA*)&buff[0])) {
        cout << "Ошибка инициализации! \n" << WSAGetLastError();
        return -1;
    }

    SOCKET sListener, sNew;
    sockaddr_in sin, clntSin;

    sListener = socket(AF_INET, SOCK_STREAM, 0);
    if (sListener == INVALID_SOCKET) {
        cout << "Ошибка создания сокета! \n" << WSAGetLastError();
        WSACleanup();
        return -1;
    }

    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;  // Принимать подключения на все интерфейсы
    sin.sin_port = htons(SRV_PORT);

    if (bind(sListener, (sockaddr*)&sin, sizeof(sin)) == SOCKET_ERROR) {
        cout << "Ошибка привязки сокета! \n" << WSAGetLastError();
        closesocket(sListener);
        WSACleanup();
        return -1;
    }

    // Выводим информацию о сети
    printNetworkInfo();

    int len;
    char buf[BUF_SIZE] = { 0 };
    string msg;

    if (listen(sListener, 5) == SOCKET_ERROR) {
        cout << "Ошибка прослушивания! \n" << WSAGetLastError();
        closesocket(sListener);
        WSACleanup();
        return -1;
    }

    cout << "Сервер запущен и ожидает подключений..." << endl;

    while (true) {
        int clntLen = sizeof(clntSin);
        sNew = accept(sListener, (sockaddr*)&clntSin, &clntLen);
        if (sNew == INVALID_SOCKET) {
            cout << "Ошибка принятия соединения! \n" << WSAGetLastError();
            continue;
        }

        // Получаем информацию о клиенте
        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clntSin.sin_addr), clientIP, INET_ADDRSTRLEN);
        cout << "\n>>> Новый клиент подключился! IP: " << clientIP << endl;

        string greeting = "Сервер готов к расчету премий";
        int msgSize = static_cast<int>(greeting.size());
        send(sNew, greeting.c_str(), msgSize, 0);

        while (true) {
            // Получаем данные сотрудника от клиента
            len = recv(sNew, buf, BUF_SIZE, 0);
            if (len == SOCKET_ERROR || len == 0) {
                cout << "Клиент отключился: " << clientIP << endl;
                break;
            }

            if (len < static_cast<int>(sizeof(Employee))) {
                cout << "Получены неполные данные от клиента" << endl;
                continue;
            }

            Employee* emp = (Employee*)buf;
            string bonusCode = calculateBonus(*emp);

            cout << "Обработан сотрудник " << emp->name << " от клиента " << clientIP
                << ": " << bonusCode << endl;

            // Отправляем код премии
            int bonusSize = static_cast<int>(bonusCode.size());
            int sent = send(sNew, bonusCode.c_str(), bonusSize, 0);
            if (sent == SOCKET_ERROR) {
                cout << "Ошибка отправки данных клиенту" << endl;
                break;
            }
        }

        closesocket(sNew);
        cout << "Соединение с клиентом закрыто" << endl;
    }

    closesocket(sListener);
    WSACleanup();
    return 0;
}