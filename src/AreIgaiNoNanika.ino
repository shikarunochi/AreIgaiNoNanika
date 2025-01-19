#include <SD.h>
#include <M5Unified.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
// https://lang-ship.com/blog/work/esp32root-ca-https/
// こちらを参考に、x509_crt_bundle.h と x509_crt_bundle.cpp を作成してください。
#include "x509_crt_bundle.h"

//プログラムで固定指定する場合は入れてください。
//入れてない場合、前回接続SSIDで接続、または SmartConfig待受が起動します。
const char *SSID = NULL;  
const char *PASSWORD = NULL;

#define MAX_DATA_COUNT 100
String rssUrlList[MAX_DATA_COUNT];
int rssCount = 0;
String wordList[MAX_DATA_COUNT];
int wordCount = 0;
String topicsWordList[MAX_DATA_COUNT];
int topicsWordCount = 0;

String checkWordList[] = {"死", "戦", "殺", "災",""}; // 該当の文字が入っていた時は、ツッコミを固定にしておく

int rssUrlIndex = 0;

#define TITLE_COUNT_MAX 50

M5Canvas textCanvas(&M5.Lcd);

String titleTextList[TITLE_COUNT_MAX];
WiFiClientSecure *client;

void getRSS(String URL)
{
	Serial.printf("get RSS from:[%s]\n", URL.c_str());
	for (int i = 0; i < TITLE_COUNT_MAX; i++)
	{
		titleTextList[i] = "";
	}

	if (client)
	{
		HTTPClient https;
		if (https.begin(*client, rssUrlList[rssUrlIndex]))
		{
			int httpCode = https.GET();
			if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
			{
				String payload = https.getString();
				int titleIndex = 0;
				int offset = 0;
				//<title>と</title>の間を取得
				while (true)
				{
					int start = payload.indexOf("<title>", offset);
					int end = payload.indexOf("</title>", offset);
					if (start < 0)
					{
						break;
					}
					start = start + 7; //<title>文字数を足しておく
					// 先頭が"["だった場合、"]"までの間を削除。"]"以降を表示する
					if (payload.charAt(start) == '[')
					{
						int bracketEnd = payload.indexOf(']', offset);
						if (bracketEnd > 0)
						{
							start = bracketEnd + 1;
						}
					}
					titleTextList[titleIndex] = payload.substring(start, end);
					titleTextList[titleIndex].trim();
					Serial.println(titleTextList[titleIndex]);
					offset = end + 7;
					titleIndex++;
					if (titleIndex >= TITLE_COUNT_MAX)
					{
						break;
					}
				}
				Serial.printf("read: %d\n",titleIndex);
			}
			else
			{
				Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
			}
			https.end();
		}
		else
		{
			Serial.printf("[HTTPS] Unable to connect\n");
		}
	}
}

void drawSerif(String mainText, String subText)
{

	M5.Lcd.fillRoundRect(10, 10, 180, 150, 20, WHITE);
	M5.Lcd.drawRoundRect(10, 10, 180, 150, 20, BLACK);
	M5.Lcd.fillTriangle(189, 100, 200, 110, 189, 110, WHITE);
	M5.Lcd.drawLine(189, 100, 200, 110, BLACK);
	M5.Lcd.drawLine(200, 110, 189, 110, BLACK);

	textCanvas.fillScreen(WHITE);
	Serial.printf("mainText:%d[%s]\n", mainText.length(), mainText.c_str());
	if (mainText.length() >= 130)
	{
		textCanvas.setTextSize(0.8);
	}
	else if (mainText.length() >= 95)
	{
		textCanvas.setTextSize(0.9);
	}
	else if (mainText.length() >= 50)
	{
		textCanvas.setTextSize(1.0);
	}
	else
	{
		textCanvas.setTextSize(1.4);
	}
	textCanvas.setCursor(0, 0);
	textCanvas.println(decodeEscape(mainText));
	textCanvas.pushRotateZoomWithAA(100, 80, 0, 0.5, 0.5);

	M5.Lcd.clearClipRect();
	M5.Lcd.fillRoundRect(20, 170, 150, 60, 20, WHITE);
	M5.Lcd.drawRoundRect(20, 170, 150, 60, 20, BLACK);
	M5.Lcd.fillTriangle(169, 190, 179, 200, 169, 200, WHITE);
	M5.Lcd.drawLine(169, 190, 179, 200, BLACK);
	M5.Lcd.drawLine(179, 200, 169, 200, BLACK);
	M5.Lcd.setClipRect(20, 175, 140, 50);
	// 長さによって上か真ん中かぐらいは決める
	Serial.printf("subText:%d[%s]\n", subText.length(), subText.c_str());
	textCanvas.fillScreen(WHITE);
	textCanvas.setTextSize(1.2);
	if (subText.length() <= 18)
	{
		textCanvas.setCursor(0, 30);
	}
	else
	{
		textCanvas.setCursor(0, 0);
	}
	textCanvas.println(subText);
	textCanvas.pushRotateZoomWithAA(98, 230, 0, 0.45, 0.45);
	M5.Lcd.clearClipRect();
}

String decodeEscape(String text)
{
	// text.replace("&lt;","<");
	// text.replace("&gt;",">");
	// text.replace("&quot;","\"");
	text.replace("&amp;", "&");
	return text;
}

void setup()
{
	auto cfg = M5.config();
	M5.begin(cfg);
	Serial.begin(115200);
	delay(100);
	while (false == SD.begin(GPIO_NUM_4, SPI, 25000000))
	{
		delay(500);
	}
	M5.Lcd.drawJpgFile(SD, "/nanika.jpg");

	textCanvas.setTextColor(BLACK, WHITE);
	textCanvas.setFont(&fonts::lgfxJapanGothicP_40);

	textCanvas.setColorDepth(8);
	textCanvas.createSprite(150 * 2, 126 * 2);

	drawSerif("Wi-Fi\n接続中！", "しばし\n待たれよ…");
	
	if(SSID != NULL){
		WiFi.begin(SSID,PASSWORD);
	}else{
		WiFi.begin();//前回設定時の情報で接続する。
	}
		// 接続完了まで待機.
		int WiFiCount = 0;
		M5.Lcd.setTextColor(BLACK, WHITE);
		M5.Lcd.setTextSize(2);
		while (WiFi.status() != WL_CONNECTED)
		{
			Serial.print(".");
			M5.Lcd.setCursor(30 + WiFiCount * 16, 90);
			M5.Lcd.print("o");
			delay(500);
			M5.Lcd.setCursor(30 + WiFiCount * 16, 90);
			M5.Lcd.print(" ");
			WiFiCount++;
			if (WiFiCount > 5)
			{
				WiFiCount = 0;
			}
			// 10秒以上接続できなかったら抜ける
			if ( 10000 < millis() ) {
				break;
			}
		}
		// 未接続の場合にはSmartConfig待受
		// https://lang-ship.com/blog/work/esp32-wi-fi-setting/#toc9
		if ( WiFi.status() != WL_CONNECTED ) {
			WiFi.mode(WIFI_STA);
			WiFi.beginSmartConfig();
			Serial.println("Waiting for SmartConfig");
			drawSerif("SmartConfigで\nWi-Fi設定してください！", "");
			while (!WiFi.smartConfigDone()) {
				delay(500);
				Serial.print("#");
				// 30秒以上接続できなかったら抜ける
				if ( 30000 < millis() ) {
					Serial.println("");
					Serial.println("Reset");
					drawSerif("SmartConfig失敗。\n再起動します。", "ムムム…");
					delay(2000);
					ESP.restart();
				}
			}	
			// Wi-fi接続
			Serial.println("");
			Serial.println("Waiting for WiFi");
			drawSerif("Wi-Fi\n接続中！", "しばし\n待たれよ…");
			while (WiFi.status() != WL_CONNECTED) {
			delay(500);
			Serial.print(".");
			// 60秒以上接続できなかったら抜ける
			if ( 60000 < millis() ) {
				Serial.println("");
				Serial.println("Reset");
				drawSerif("Wi-Fi接続失敗。\n再起動します。", "あれれ～");
				delay(2000);
				ESP.restart();
			}
			Serial.println("");
			Serial.println("WiFi Connected.");
		}
	}

	client = new WiFiClientSecure;
	if (client)
	{
		client->setCACertBundle(x509_crt_bundle);
	}
	else
	{
		M5.Lcd.setCursor(30, 100);
		M5.Lcd.setTextSize(3);
		M5.Lcd.print("WiFiClientError!");
		while(true){
			delay(1000);
		}
	}

	Serial.println(" connected");
	rssCount = readConfFile("/RssUrlList.dat",rssUrlList,true);
	wordCount = readConfFile("/SerifWordList.dat",wordList,false);
	topicsWordCount = readConfFile("/TopicsWordList.dat",topicsWordList,false);
	
}

int readConfFile(String fileName, String targetList[],bool forRSS){
	File fp = SD.open(fileName, "r");
	String buf = "\0";
	int lineCount = 0;
	while (fp && fp.available())
	{
		char nextChar = char(fp.read());
		// 改行か"#"が出るまで読む。
		if (nextChar == '\r' || nextChar == '\n' || nextChar == '#')
		{
			if (buf.length() == 0)
			{ // バッファまだ0文字の場合、何もせずもう一度処理
				continue;
			}
			if (nextChar == '#' && forRSS)
			{ // #の場合は以降はコメントなので行末まで読み飛ばし。RSSの場合のみ
				while (fp.available())
				{
					if (char(fp.read()) == '\r')
					{
						break;
					}
				}
			}
			if( forRSS == false){
				buf.replace("\\n","\n"); //突込みセリフの場合は改行処理
			}
			buf.trim();
			targetList[lineCount] = buf;
			lineCount++;
			buf = "\0";
		}
		else
		{
			buf += nextChar;
		}
	}
	return lineCount;
}

void loop()
{
	M5.Lcd.setTextSize(1);
	M5.Lcd.setCursor(35, 145);
	M5.Lcd.println(">>Getting Next Topics...");

	getRSS(rssUrlList[rssUrlIndex]);

	int titleIndex = 0;
	while (true)
	{
		if (titleTextList[titleIndex] == "" || titleIndex >= TITLE_COUNT_MAX)
		{
			break;
		}
		if (titleIndex == 0)
		{
			int wordIndex = random(topicsWordCount);
			drawSerif(titleTextList[titleIndex], topicsWordList[wordIndex]);
		}
		else
		{
			int checkIndex = 0;
			bool checkWordFlag = false;
			while (true)
			{
				if (checkWordList[checkIndex] == "")
				{
					break;
				}
				if (titleTextList[titleIndex].indexOf(checkWordList[checkIndex]) >= 0)
				{
					checkWordFlag = true;
					break;
				}
				checkIndex++;
			}
			if (checkWordFlag == false)
			{
				int wordIndex = random(wordCount);
				drawSerif(titleTextList[titleIndex], wordList[wordIndex]);
			}
			else
			{
				drawSerif(titleTextList[titleIndex], "そっか…");
			}
		}
		delay(5000);
		titleIndex++;
	}
	rssUrlIndex++;
	if (rssUrlIndex >= rssCount)
	{
		rssUrlIndex = 0;
	}
}