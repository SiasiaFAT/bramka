#include "mbed.h"
#include "function.h"
#include "NTPClient.h"
#include "EthernetInterface.h"
#include "HTTPClient.h"
#include <iostream>;
 
int _id_card=0;                                     // zmienna przechowujaca numer karty
DigitalIn Change_Read_Write(p29);
DigitalOut led1(LED1); // use for debug
DigitalOut led2(LED2); // use for debug
DigitalOut led3(LED3); // use for debug
DigitalOut led4(LED4); // use for debug
bool internetA = 0;
bool internetB = 0;
bool internetC = 1;
int sockConnA = -1;
int sockConnB = -1;
bool err = false;
float aproxCycleTime = 0;
float cyclesIn30sec = 0;
int i = 0;

NTPClient ntp;

bool state_Read_Write;

int main()
{
    err = false;
    sockConnA = -1;
    sockConnB = -1;
    internetA = 1;
    internetB = 0;
    internetC = 1;
    aproxCycleTime = 0.3; // zmienna ustawiajaca ile ma trwac jedno przejscie cyklu (~jedno mrugniecie diody LED3)
    cyclesIn30sec = 30/aproxCycleTime;
    i=0;
    
    state_Read_Write=0;
    pc.printf("Starting... ");
    char buffer[40];
    
    // stworz polaczenie z netem, polacz, ustaw czas
    eth.init(); //DHCP
    eth.connect();
    
    led1=1;
    
    if (ntp.setTime("0.pl.pool.ntp.org") == 0)
    {
        pc.printf("Set time successfully\r\n");
        time_t ctTime;
        ctTime = time(NULL);
        if ((ctTime>1490486400&&ctTime<1509235200)||(ctTime>1521936000&&ctTime<1540684800)||(ctTime>1553990400&&ctTime<1572134400)||(ctTime>1585440000&&ctTime<1603584000)||
             (ctTime>1616889600&&ctTime<1635638400)||(ctTime>1648339200&&ctTime<1667088000)||(ctTime>1679788800&&ctTime<1698537600)||(ctTime>1711843200&&ctTime<1729987200)||
             (ctTime>1743292800&&ctTime<1761436800)||(ctTime>1774742400&&ctTime<1792886400)||(ctTime>1806192000&&ctTime<1824940800)||(ctTime>1837641600&&ctTime<1856390400)||
             (ctTime>1869091200&&ctTime<1887840000)||(ctTime>1901145600&&ctTime<1919289600)){
            ctTime = time(NULL)+7200; // dodanie 2 godzin do czasu z serwera
        }else{
            ctTime = time(NULL)+3600; // dodanie 1 godziny do czasu z serwera
        }
        printf("Time is set to: %s\r\n", ctime(&ctTime));
        strftime(buffer,40,"%Y-%m-%d %H:%M:%S",localtime(&ctTime)); // ustawienie formatu daty
        pc.printf("%s\r\n",buffer);
        //jesli zasilanie wznowione, a nie bylo polaczenia z internetem (sterownik ustawi sie na 01.01.1970 00:59:59)
        //sygnalizuj, ze cos jest nie tak - wlacz LED1 i zablokuj mozliwosc wejscia wszystkim (drzwi i kolowrotek nie beda reagowac na przylozenie karty)
        pc.printf("%d\r\n",ctTime);
        if (ctTime<946708560){ // jezeli czas jest mniejszy niz rok 2000 (nie naprawia tego przez 30 lat, to klops xD)
            err=true;
        }
    }
    else
    {
      pc.printf("Error\r\n");
    } 
    eth.disconnect();  
    
    eth.connect();
    // po wlaczeniu sterownika nastapi zrzut z buffora SD, jesli jest polaczenie z internetem. Plik buffora nie zostanie skasowany, jesli nie ma polaczenia z socketem
    if(ether.link()){
        pc.printf("FUNKCJA SEND TO DB WLACZONA!!!");
        send_to_DB();
    }
    
    eth.connect();
//    
//    
//    
//    
//    
    // rozpocznij wieczna petle (czeka na przylozenie karty itd.)
    while(1) {
    
        led2=1;
        //jesli zegar sterownika zostal zle ustawiony (podczas wznowienia zasilania nie bylo internetu), 
        //to sygnalizuj blad i zablokuj mozliwosc wykonywania dalszego programu
        if (err==true){
            led1=1;
            Door_open_1=0;
            Door_open_2=0;
            Door_open_3=0;
            while(1){
                pc.printf("WRONG TIME - RESTART ME WHEN INTERNET IS ON:)   ");
                wait(60);
                NVIC_SystemReset();
            }
        }
        
        //jezeli zmienil sie status internetu z 0 na 1 (internetA=0, internetB=1) i mozna polaczyc sie z socketem, to wykonaj zrzut pliku buffor.txt do bazy, a po zrzuceniu nadpisz buffor.txt pustym plikiem
        if((internetA==0&&internetB==1)||(internetA==1&&internetC==0)||(sockConnA==0&&sockConnB==-1)){
            int conn = sock.connect("192.168.90.123", 80);
            if(conn==0){
                pc.printf("FUNKCJA SEND TO DB WLACZONA!!!");
                send_to_DB();
            }
            sock.close();
        }
        
        //sprawdz czy jest polaczenie z socketem
        sockConnB = sockConnA; 
        pc.printf(" B=");
        pc.printf("%d",sockConnB);
        
        //raz na pol minuty polacz z socketem (wznowi polaczenie z serwerem, jesli to zostanie przerwane)
        if(i==static_cast<int>(cyclesIn30sec)){
            i=0;
            if(ether.link()){
                pc.printf(" lacze z socketem ");
                sockConnA = sock.connect("192.168.90.123", 80);
                cout << sockConnA << endl;
                pc.printf(" A=");
                pc.printf("%d",sockConnA);
                if(sockConnA==0){
                    pc.printf(" POLACZYLEM ");
                }else{
                    pc.printf(" NIE MA SOCKETA ");
                }
                sock.close();
            }else{
                // jesli nie ma polaczenia z netem, to nie ma sensu sprawdzac polaczenia z socketem! (Bedzie mocno opozniac sterownik)
                pc.printf(" NIE MA NETA! ");
            }
        }
        i++;
        
        if(internetA==0&&internetB==0){
            internetC=0;
        }else{
            internetC=1;
        }
            
        //sprawdz czy jest internet
        if(ether.link()){
            internetA=1;
        }else{
            internetA=0;
        }
        //pc.printf(" A=");
        //pc.printf("%d",internetA);
        //pc.printf("%d",internetB);
        
        led3=1;
        wait(aproxCycleTime);
        led3=0;
        
        //czy jest wcisniety przycisk zapisu (zapis nr karty na SD - te karty beda otwierac drzwi)
        if(Change_Read_Write == 1) {
            led2 = 0;
            wait(0.2);
            state_Read_Write = !state_Read_Write;
            while(Change_Read_Write == 1){
                if(state_Read_Write == 1) {
                    Write_lamp = 1;
                }else{
                    Write_lamp = 0;
                }
                wait(0.2);
            }
        }
        else{
            if(state_Read_Write == 1) {
                    Write_lamp = 1;
            }else{
                    Write_lamp = 0;
                }
            wait(0.2);
        }  
        
        // odczyt z karty WEJSCIE 1         
            if(rfid_10.readable()) {                // WEJSCIE 1 warunek sprawdzajacy czy do czujnika zostala przylozona karta
                 pc.printf("przylozenie karty");
                _id_card = rfid_10.read();          // odczyt numeru karty
                rfid_27.read();
                rfid_14.read();
                serial_bridge.read_from();
                 pc.printf("odczyt numeru");
                led4=1;
                wait(0.2);
                led4=0;
                if(state_Read_Write == 0) {
                    printf("RFID Tag number : %d\n", _id_card);
                    if(_id_card!=123){                                    // jesli funkcja read() zwrocila 123, to czytnik przeczytal jakies bzdury
                        ReadFromSD(_id_card,"wejscie","1");               // funkcja sprawdzajaca numer karty w pliku access.txt
                        pc.printf("porownanie z plikiem SD access");
                        if(ether.link()){                                 //sprawdz czy jest internet, jesli jest to wyslij na serwer, jesli nie to na SD do pliku buffer
                            pc.printf("WYSYLA NA SERWER");
                            send_to_server(_id_card,"wejscie","1");       // wysylanie odczytanych informacji na serwer do bazy danych FAT.sqlite           
                        }else{
                            pc.printf("WYSYLA NA SD");
                            send_to_SD(_id_card,"wejscie","1");
                            pc.printf("WYSYLALO NA SD!!!"); 
                        }
                    }else{
                        pc.printf("rubbish detected");
                    }   
                    wait(0.5);
                    Door_open_1=0;
                    Door_open_2=0;
                    Door_open_3=0;
                }
                if(state_Read_Write == 1) {
                    Write_lamp = 0;
                    if(_id_card!=123){
                        if(ether.link()){ //sprawdz czy jest internet, jesli jest to wyslij na serwer, jesli nie to zolta dioda zasygnalizuj blad (trzy razy mrugnij)
                            WriteToSD(_id_card);                           
                            send_to_server(_id_card,"nowy_pracownik","1");
                        }else{
                            Write_lamp = 0;
                            wait(0.3);
                            Write_lamp = 1;
                            wait(0.3);
                            Write_lamp = 0;
                            wait(0.3);
                            Write_lamp = 1;
                            wait(0.3);
                            Write_lamp = 0;
                            wait(0.3);
                        }
                    }else{
                        pc.printf(" rubbish detected ");
                    } 
                    Write_lamp = 1;
                }
            }
            
        // odczyt z karty WYJSCIE 1  
            if(rfid_27.readable()) { // WYJSCIE 1
                _id_card =rfid_27.read();
                rfid_10.read();
                rfid_14.read();
                serial_bridge.read_from();
                led4=1;
                wait(0.2);
                led4=0;
                printf("RFID Tag number : %d\n", _id_card );
                if(_id_card!=123){
                    ReadFromSD(_id_card,"wyjscie","1");
                    pc.printf(" przeczytanie z SD ");
                    if(ether.link()){ //sprawdz czy jest internet, jesli jest to wyslij na serwer, jesli nie to na SD do pliku buffer
                        pc.printf(" WYSYLA NA SERWER ");
                        send_to_server(_id_card,"wyjscie","1"); // wysylanie odczytanych informacji na serwer do bazy danych FAT.sqlite
                        pc.printf(" WYSYLALO NA SERWER!!! ");               
                    }else{
                        pc.printf(" WYSYLA NA SD ");
                        send_to_SD(_id_card,"wyjscie","1");
                        pc.printf(" WYSYLALO NA SD!!! "); 
                    }
                }else{
                    pc.printf(" rubbish detected ");
                }  
                wait(0.5);
                Door_open_1=0;
                Door_open_2=0;
                Door_open_3=0;
            }
            
       // odczyt z karty WEJSCIE 2       
            if(rfid_14.readable()) {   // WEJSCIE 2
                _id_card =rfid_14.read();
                rfid_10.read();
                rfid_27.read();
                serial_bridge.read_from();
                led4=1;
                wait(0.2);
                led4=0;
                printf("RFID Tag number : %d\n", _id_card );
                if(_id_card!=123){
                    ReadFromSD(_id_card,"wejscie","2");
                    if(ether.link()){ //sprawdz czy jest internet, jesli jest to wyslij na serwer, jesli nie to na SD do pliku buffer
                            pc.printf(" WYSYLA NA SERWER ");
                            send_to_server(_id_card,"wejscie","2"); // wysylanie odczytanych informacji na serwer do bazy danych FAT.sqlite
                            pc.printf(" WYSYLALO NA SERWER!!! ");               
                    }else{
                            pc.printf(" WYSYLA NA SD ");
                            send_to_SD(_id_card,"wejscie","2");
                            pc.printf(" WYSYLALO NA SD!!! "); 
                    }
                }else{
                    pc.printf(" rubbish detected ");
                } 
                wait(0.5);
                Door_open_1=0;
                Door_open_2=0;
                Door_open_3=0;
            }
            
        // odczyt z karty WYJSCIE 2     
            if(serial_bridge.readable()) {  //WYJSCIE 2
                wait(0.1);
                led4=1;
                wait(0.2);
                led4=0;
                _id_card = serial_bridge.read_from();
                rfid_10.read();
                rfid_14.read();
                rfid_27.read();
                printf("RFID Tag number : %d\n", _id_card );
                if(_id_card!=123){
                    ReadFromSD(_id_card,"wyjscie","2");
                    if(ether.link()){ //sprawdz czy jest internet, jesli jest to wyslij na serwer, jesli nie to na SD do pliku buffer
                            pc.printf(" WYSYLA NA SERWER ");
                            send_to_server(_id_card,"wyjscie","2"); // wysylanie odczytanych informacji na serwer do bazy danych FAT.sqlite
                            pc.printf(" WYSYLALO NA SERWER!!! ");               
                    }else{
                            pc.printf(" WYSYLA NA SD ");
                            send_to_SD(_id_card,"wyjscie","2");
                            pc.printf(" WYSYLALO NA SD!!! ");
                    }
                }else{
                    pc.printf(" rubbish detected ");
                }
                wait(0.5);
                Door_open_1=0;
                Door_open_2=0;
                Door_open_3=0;
            }
            if(ether.link()){
                internetB=1;
            }else{
                internetB=0;
            }
            //pc.printf(" B=");
            //pc.printf("%d",internetB);
        }
}