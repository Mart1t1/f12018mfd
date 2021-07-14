#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <pthread.h>
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
//#include <SDL2/SDL_ttf.h>



#include "structs.h"


#define BUF_SIZE 1500


SDL_Surface *screen;
SDL_Surface *image;

#define WIDTH   800
#define HEIGHT  480


void wait_for_keypressed()
{
    SDL_Event event;

    // Wait for a key to be down.
    do
    {
        SDL_PollEvent(&event);
    } while(event.type != SDL_KEYDOWN);

    // Wait for a key to be up.
    do
    {
        SDL_PollEvent(&event);
    } while(event.type != SDL_KEYUP);
}


void error_handling(char *message);

void drawText(char* string,int size, int x, int y)
{
    
    TTF_Font* font = TTF_OpenFont("deffon.ttf", size);

    SDL_Color foregroundColor = {255,255,255};
    SDL_Color backgroundColor = {125,125,125};

    SDL_Surface* textSurface = TTF_RenderText_Blended(font, string, foregroundColor);
    

    SDL_Rect textLocation = { x, y, 0, 0 };
    

    SDL_BlitSurface(textSurface, NULL, screen, &textLocation);

    SDL_FreeSurface(textSurface);

    TTF_CloseFont(font);

    


}

char *my_itoa(int num, char *str)
{
        if(str == NULL)
        {
                return NULL;
        }
        sprintf(str, "%d", num);
        return str;
}


void drawScreen(struct Info* info)
{
    char str[20];
    
    SDL_FillRect(screen, NULL, 0);

    
    //memset(str, 0, sizeof(str));
    my_itoa(info->speed, str);
    drawText(str, 50, 0, 0);
    
    //memset(str, 0, sizeof(str));
    my_itoa(info->gear, str);
    drawText(str, 100, WIDTH/2, HEIGHT/2);
    
    //memset(str, 0, sizeof(str));
    my_itoa(info->engineRPM, str);
    drawText(str, 50, 0, HEIGHT/2 + 50);
    
    SDL_Flip(screen);
    
    
}


void createScreen()
{
    
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    screen = SDL_SetVideoMode(WIDTH, HEIGHT, 0, SDL_ANYFORMAT);
    
    SDL_Color fore = {0,0,0};
    
    SDL_FillRect(screen, NULL, 0);
    SDL_Flip(screen);
    

}




void updatetelemetry(struct PacketCarTelemetryData* packet, struct Info* info, uint8_t playerIndex)
{
    
    info->gear = packet->m_carTelemetryData[playerIndex].m_gear;
    info->engineRPM = packet->m_carTelemetryData[playerIndex].m_engineRPM;
    info->drs = packet->m_carTelemetryData[playerIndex].m_drs;
    info->speed = packet->m_carTelemetryData[playerIndex].m_speed;
    info->revLightsPercent = packet->m_carTelemetryData[playerIndex].m_revLightsPercent;
    
    printf("\n\tgear : %d\n\n", packet->m_carTelemetryData[playerIndex].m_gear);
    
    
}

void updateLapData(struct PacketLapData* packet, struct Info* info, uint8_t playerIndex)
{
    info->currentLapTime = packet->m_lapData[playerIndex].m_currentLapTime;
    info->lastLapTime = packet->m_lapData[playerIndex].m_lastLapTime;
}

void processMessage(char* message, int len, struct Info* info)
{
    char types[8][15] = {"Motion", "Session", "Lapdata", "Event", "Participants", "Car Setups", "Car Telemetry", "Car Status"};
    struct PacketHeader* header = (struct PacketHeader*)message;
    
    int PacketID = header->m_packetId;
    
    int8_t playerIndex = header->m_playerCarIndex;
    
    
    if(PacketID == 6)
        updatetelemetry((struct PacketCarTelemetryData*)message, info, playerIndex);
    
    if(PacketID == 2)
        updateLapData((struct PacketLapData*)message, info, playerIndex);
    
    drawScreen(info);
    
}

void* datanaAlysis(void* pi)
{
    int *p = pi;
    char message[BUF_SIZE];
    
    struct Info info;
    
    createScreen();
    
    info.gear = 0;
    info.engineRPM = 0;
    info.drs = 0;
    info.speed = 0;
    info.revLightsPercent = 0;
    
    info.currentLapTime = 0;
    info.lastLapTime = 0;

    while (1)
    {
        memset(&message, 0, BUF_SIZE);
        int len = read(p[0], message, BUF_SIZE);
        if(len && len != 1500)
        {
            processMessage(message, len, &info);

        }
        

    }
    
    return NULL;
    
}

int main(int argc, char *argv[]){
    
    int p[2];
    pthread_t dataAnalysisThread;
    int serv_sock;
    char message[BUF_SIZE];
    int str_len;
    socklen_t clnt_adr_sz;

    pipe(p);
    
    pthread_create(&dataAnalysisThread, NULL, datanaAlysis, (void *)p);

    struct sockaddr_in serv_adr, clnt_adr;
    if(argc != 2){
        printf("Usage : %s <port>\n",argv[0]);
        exit(1);
    }
    serv_sock = socket(PF_INET, SOCK_DGRAM, 0);
    if(serv_sock == -1)
        error_handling("UDP socket creation error");
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    if(bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr))== -1)
        error_handling("bind() error");
    

    
    while(1){
        clnt_adr_sz = sizeof(clnt_adr);
        str_len = recvfrom(serv_sock, message, BUF_SIZE, 0,(struct sockaddr*)&clnt_adr, &clnt_adr_sz);
        write(p[1], message, str_len);
        
    }
    close(serv_sock);
    return 0;
}

void error_handling(char *message){
    fputs(message, stderr);
    fputc('\n',stderr);
    exit(1);
}
