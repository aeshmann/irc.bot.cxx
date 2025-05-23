#include <iostream>
#include <cstring>
#include <netdb.h>
#include <vector>
#include <string>

#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <curl/curl.h>

#include "cppjson.h"
#include "ircbot.h"


// Функция обратного вызова для записи данных из ответа сервера
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
	size_t totalSize = size * nmemb;
	userp->append((char*) contents, totalSize);
	return totalSize;
}

// Принимает строку с ip адресом, возвращает ответ с ipinfo.io
std::string getIpInfo(std::string /* string with ip */ ipAddrStr, std::string ipinfo_token)
{

	CURL* curl;
	CURLcode res;

	std::string ipInfoStr;
	std::string ipReadStr;
	std::string readbuffer;
	std::string requeststr;

	curl = curl_easy_init();
	if (curl)
	{
		if (!ipAddrStr.empty())
		{
			requeststr = "http://ipinfo.io/" + ipAddrStr + "?token=" + ipinfo_token;
			curl_easy_setopt(curl, CURLOPT_URL, requeststr.c_str());
		}
		else
		{
			curl_easy_setopt(curl, CURLOPT_URL, "http://ipinfo.io/");
		}

		// Установка функции обратного вызова для записи данных
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);

		// Передача указателя на строку, куда будут записываться данные
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readbuffer);

		// Выполнение запроса
		res = curl_easy_perform(curl);

		// Проверка результата выполнения запроса
		if (res != CURLE_OK)
		{
			std::string curl_err(curl_easy_strerror(res));
			ipInfoStr = "curl_easy_perform() failed: " + curl_err + '\n';
		}
		else
		{
			// Возврат строки с ipinfo.io
			ipInfoStr = readbuffer;
			std::cout << "IP info string:\n" << ipInfoStr << '\n';
			nlohmann::json jsonData = nlohmann::json::parse(ipInfoStr);

			if (!jsonData["ip"].is_null())
			{
				ipReadStr += std::string("\x02") + "IP:      " + std::string("\x03") + "03" + jsonData["ip"].get<std::string>() + "\x03" + '\n';
			}

			if (!jsonData["hostname"].is_null())
			{
				ipReadStr += "Host:    " + jsonData["hostname"].get<std::string>() + '\n';
			}

			if (!jsonData["city"].is_null())
			{
				ipReadStr += "City:    " + jsonData["city"].get<std::string>() + '\n';
			}

			if (!jsonData["region"].is_null())
			{
				ipReadStr += "Region:  " + jsonData["region"].get<std::string>() + '\n';
			}

			if (!jsonData["country"].is_null())
			{
				ipReadStr += "Country: " + jsonData["country"].get<std::string>() + '\n';
			}

			if (!jsonData["loc"].is_null())
			{
				ipReadStr += "Loc:     " + jsonData["loc"].get<std::string>() + '\n';
			}

			if (!jsonData["org"].is_null())
			{
				ipReadStr += "Org:     " + jsonData["org"].get<std::string>() + '\n';
			}

			if (!jsonData["postal"].is_null())
			{
				ipReadStr += "Index:   " + jsonData["postal"].get<std::string>() + '\n';
			}

			if (!jsonData["timezone"].is_null())
			{
				ipReadStr += "Tzone:   " + jsonData["timezone"].get<std::string>() /* + '\n' */;
			}
		}
		// Освобождение ресурсов
		curl_easy_cleanup(curl);
	}
	else
	{
		std::cerr << "Failed to initialize libcurl." << std::endl;
	}
	return ipReadStr;
}

// Принимает строку с хостнеймом, возвращает вектор строк с ip адресом
std::vector<std::string> getIpAddr(const std::string& hostname) {
	std::vector<std::string> ipAddrSet;
	struct addrinfo hints = {};
	struct addrinfo* res = nullptr;

	// Настройка параметров для getaddrinfo
	hints.ai_family = AF_UNSPEC;    // IPv4 или IPv6
	hints.ai_socktype = SOCK_STREAM; // TCP

	int status = getaddrinfo(hostname.c_str(), nullptr, &hints, &res);
	if (status != 0) {
		std::cerr << "getaddrinfo error: " << gai_strerror(status) << std::endl;
		return ipAddrSet; // возвращаем пустой вектор при ошибке
	}

	// Перебор всех адресов, соответствующих имени хоста
	for (struct addrinfo* p = res; p != nullptr; p = p->ai_next) {
		void* addr;
		//std::string ipver;

		// Определение типа адреса (IPv4 или IPv6)
		if (p->ai_family == AF_INET) { // IPv4
			struct sockaddr_in* ipv4 = reinterpret_cast<struct sockaddr_in*>(p->ai_addr);
			addr = &(ipv4->sin_addr);
			//ipver = "IPv4";
		}
		else if (p->ai_family == AF_INET6) { // IPv6
			struct sockaddr_in6* ipv6 = reinterpret_cast<struct sockaddr_in6*>(p->ai_addr);
			addr = &(ipv6->sin6_addr);
			//ipver = "IPv6";
		}
		else {
			continue; // Пропускаем неизвестные типы адресов
		}

		// Преобразование адреса в строку
		char ipstr[INET6_ADDRSTRLEN];
		inet_ntop(p->ai_family, addr, ipstr, sizeof(ipstr));
		//std::cout << ipver << " address: " << ipstr << '\n' << getIpInfo(ipstr) << std::endl;
		ipAddrSet.push_back(std::string(ipstr));
	}

	freeaddrinfo(res); // Освобождение памяти
	return ipAddrSet;
}