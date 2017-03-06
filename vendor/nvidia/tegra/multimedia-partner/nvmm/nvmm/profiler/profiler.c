// profiler.c
//

#include "stdio.h"
#include "string.h"

#define MAX_FRAMES_PER_PATH   100
#define MAX_STAGES_PER_PATH   5
#define MAX_STAGES              10
#define MAX_CPUS                2
#define MAX_PATHS             2
#define MAX_NAME                64
#define NA                      -1

typedef struct {
    int PathNum;
    int FrameNum;
    long StartTime;
    int FrameStage;
    char bReadyForNextStage;
    long EndTime;
    int TimeLeftInStage;
} SFrame;

typedef struct {
    char name[MAX_NAME];
    int PathNum;
    int Fps;
    int CurrentFrames;
    int FramesDone;
    int TotalFrames;
    SFrame Frame[MAX_FRAMES_PER_PATH];
    long TotalStages;
    int Stage[MAX_STAGES_PER_PATH];
} SPath;

typedef enum  {source,transform,sink,message}Type;

typedef struct {
    char name[MAX_NAME];
    int StageNum;
    int InputFramesMax;
    int InputFramesCurrent;
    int OutputFramesMax;
    int OutputFramesCurrent;
    int CpuNum;
    long AvgTime;
    Type StageType;
} SStage;

typedef struct {
    char name[MAX_NAME];
    int CpuNum;
    int CurrentStage;
} SCpu;

void InitStage( 
    SStage *pStage,
    char *name,
    int StageNum,
    int InputFramesMax,
    int OutputFramesMax,
    int CpuNum,
    long AvgTime,
    Type StageType)
{
    strcpy(pStage->name,name);
    pStage->StageNum = StageNum;
    pStage->InputFramesMax = InputFramesMax;
    pStage->InputFramesCurrent = 0;
    pStage->OutputFramesMax = OutputFramesMax;
    pStage->OutputFramesCurrent = OutputFramesMax;
    pStage->AvgTime = AvgTime;
    pStage->CpuNum = CpuNum;
    pStage->StageType = StageType;
}

void InitPath(
    SPath *pPath, 
    char *pName, 
    int PathNum,
    int Fps, 
    int TotalFrames,
    long TotalStages, 
    int *PathStages)
{
    int i;

    strcpy(pPath->name, pName);
    pPath->PathNum = PathNum;
    pPath->Fps = Fps;
    pPath->CurrentFrames = 0;
    pPath->FramesDone = 0;
    pPath->TotalFrames = TotalFrames;
    pPath->TotalStages = TotalStages;

    for (i=0;i<TotalFrames;i++)
    {
        pPath->Frame[i].PathNum = PathNum;
        pPath->Frame[i].FrameNum = i;
        pPath->Frame[i].FrameStage = NA;
        pPath->Frame[i].bReadyForNextStage = 1;
        pPath->Frame[i].StartTime = 0;
        pPath->Frame[i].EndTime = 0;
        pPath->Frame[i].TimeLeftInStage = NA;
    }

    for (i=0;i< TotalStages; i++)
        pPath->Stage[i] = PathStages[i];
    
}

void InitCpu(
    SCpu *pCpu,
    char *pName,
    int CpuNum)
{
    strcpy(pCpu->name, pName);
    pCpu->CpuNum = CpuNum;
    pCpu->CurrentStage = NA;
}

int main(int argc, char** argv)
{
    SPath Path[MAX_PATHS];
    SStage Stage[MAX_STAGES];
    SCpu Cpu[MAX_CPUS], *pCpu;
    SFrame *pFrame;
    SStage *pNextStage, *pPrevStage, *pStage;
    int PathsTotal = 1;
    int StagesTotal = 5;
    int CpusTotal = 2;
    int PathStages[5] = {0,1,2,3,4};
    long Clock;
    int i,j;
    char bNoWorkForCurrentClock;
    char bNoWorkAtAll;

    /*************************/
    /* SETUP                 */
    /*************************/
    
    InitStage(&Stage[0], "File Parser", 0, 4,4, 0, 10, source);
    InitStage(&Stage[1], "from File Parser to AAC Decoder", 1, 4,4, NA, 10, message);
    InitStage(&Stage[2], "AAC Decoder", 2, 4,4, 1, 10, transform);
    InitStage(&Stage[3], "from AAC Decoder to Audio Renderer", 3, 4,4, NA, 10, message);
    InitStage(&Stage[4], "Audio Renderer", 4, 4,0, 1, 10, sink);

    InitPath(&Path[0], "AAC", 0, 43, 100, 5, PathStages);

    InitCpu(&Cpu[0], "ARM11", 0);
    InitCpu(&Cpu[1], "AVP", 1);

    /*************************/
    /* EXECUTION             */
    /*************************/
    Clock = 0;

    do
    { 
        do{ 
            /* assume that there is no work */
            bNoWorkForCurrentClock =1;

            /* produce new frames */
            for (i=0; i<PathsTotal; i++)
            { 
                pStage = &Stage[Path[i].Stage[0]];
                pCpu = (pStage->CpuNum == NA)? NULL:&Cpu[pStage->CpuNum]; 
                
                /* produce a new frame? */
               if ((pStage->OutputFramesCurrent !=0) && 
                   (pCpu==NULL || pCpu->CurrentStage == NA) && 
                   (Path[i].CurrentFrames < Path[i].TotalFrames))                        
               {
                   Path[i].CurrentFrames++;
                   pFrame = &Path[i].Frame[Path[i].CurrentFrames-1];
                   pFrame->StartTime = Clock;
                   pStage->OutputFramesCurrent--;                  /* decrement available output frames */
                   if (pCpu) pCpu->CurrentStage = pStage->StageNum;    /* grab cpu for this stage */
                   pFrame->TimeLeftInStage = pStage->AvgTime;      /* start the clock */
                   pFrame->FrameStage = pStage->StageNum;

                   /* TODO: account for FPS (e.g. camera) */

                   printf("%05i:%s Frame %i started producing in %s (%s)\n", Clock, Path[i].name, 
                      pFrame->FrameNum, pStage->name, (pCpu==NULL) ? "":pCpu->name); /* todo: what if pCpu == NULL? */

                   bNoWorkForCurrentClock = 0;                     /* might be more work */
               } 
            } 

            /* service frames */
            for (i=0;i<PathsTotal;i++)
            { 
                for (j=Path[i].FramesDone; j<Path[i].CurrentFrames;j++)
                { 
                    pFrame = &Path[i].Frame[j];
                    pStage = &Stage[pFrame->FrameStage];

                    if (pStage->StageType == source)
                    { 
                        /* don't need to worry about starting a frame - that's done above */
                        pCpu = (pStage->CpuNum == NA)? NULL:&Cpu[pStage->CpuNum]; 

                        /* finish a frame? */
                        if (pFrame->TimeLeftInStage == 0)
                        { 
                            pNextStage = &Stage[Path[i].Stage[1]];
                            pFrame->FrameStage = pNextStage->StageNum;      /* move frame to next stage */
                            pFrame->TimeLeftInStage = NA;                   /* not being processed so no time left for now */
                            pNextStage->InputFramesCurrent++;               /* increment the number of next input frames */

                            pCpu->CurrentStage = NA;                       /* relinquish cpu */
                            
                            printf("%05i:%s Frame %i done producing in %s (%s)\n", Clock, Path[i].name, 
                                pFrame->FrameNum, pStage->name, (pCpu==NULL) ? "":pCpu->name);

                            bNoWorkForCurrentClock = 0;                     /* might be more work */
                        } 
       
                    } 
                    else /* stage is message */
                    if (pStage->StageType == message)
                    { 
                        /* send a new frame? */
                        if ( pFrame->TimeLeftInStage == NA &&
                            (pStage->OutputFramesCurrent != 0) &&
                            (pStage->InputFramesCurrent != 0))                        
                        { 
                            pStage->OutputFramesCurrent--;                 /* decrement available output frames */
                            pStage->InputFramesCurrent--;                  /* decrement available input frames */

                            pFrame->TimeLeftInStage = pStage->AvgTime;      /* start the clock */

                            printf("%05i:%s Frame %i message %s sent\n", Clock, Path[i].name, 
                                pFrame->FrameNum, pStage->name);

                            bNoWorkForCurrentClock = 0;                     /* might be more work */
                        } 

                        /* finish a frame? */
                        if (pFrame->TimeLeftInStage == 0)
                        { 
                            pNextStage = &Stage[Path[i].Stage[pStage->StageNum + 1]];
                            pPrevStage = &Stage[Path[i].Stage[pStage->StageNum - 1]];
                            pFrame->FrameStage = pNextStage->StageNum;      /* move frame to next stage */
                            pFrame->TimeLeftInStage = NA;                   /* not being processed so no time left for now */
                            pNextStage->InputFramesCurrent++;               /* increment the number of next input frames */
                            pPrevStage->OutputFramesCurrent++;              /* increment the number of prev input frames */
                            
                            printf("%05i:%s Frame %i message %s received\n", Clock, Path[i].name, 
                                pFrame->FrameNum, pStage->name);

                            bNoWorkForCurrentClock = 0;                     /* might be more work */
                        } 
                    } 
                    else /* stage is a transform */
                    if (pStage->StageType == transform)
                    { 
                        pCpu = (pStage->CpuNum == NA)? NULL:&Cpu[pStage->CpuNum]; 

                        /* process a new frame? */
                        if ( pFrame->TimeLeftInStage == NA &&
                            (pStage->OutputFramesCurrent != 0) &&
                            (pStage->InputFramesCurrent != 0) &&
                            (pCpu==NULL || pCpu->CurrentStage == NA))                        
                        { 
                            pStage->OutputFramesCurrent--;                 /* decrement available output frames */
                            pStage->InputFramesCurrent--;                  /* decrement available input frames */
                            if (pCpu) pCpu->CurrentStage = pStage->StageNum;    /* grab cpu for this stage */

                            pFrame->TimeLeftInStage = pStage->AvgTime;      /* start the clock */
                            
                            printf("%05i:%s Frame %i started processing in %s (%s)\n", Clock, Path[i].name, 
                                pFrame->FrameNum, pStage->name, (pCpu==NULL) ? "":pCpu->name);

                            bNoWorkForCurrentClock = 0;                     /* might be more work */            
                        } 

                        /* finish a frame? */
                        if (pFrame->TimeLeftInStage == 0)
                        { 
                            pNextStage = &Stage[Path[i].Stage[pStage->StageNum + 1]];
                            pPrevStage = &Stage[Path[i].Stage[pStage->StageNum - 1]];
                            pFrame->FrameStage = pNextStage->StageNum;      /* move frame to next stage */
                            pFrame->TimeLeftInStage = NA;                   /* not being processed so no time left for now */
                            pNextStage->InputFramesCurrent++;               /* increment the number of next input frames */
                            pPrevStage->OutputFramesCurrent++;              /* increment the number of prev input frames */
                            
                            pCpu->CurrentStage = NA;                       /* relinquish cpu */
                           
                            printf("%05i:%s Frame %i done processing in %s (%s)\n", Clock, Path[i].name, 
                                pFrame->FrameNum, pStage->name, (pCpu==NULL) ? "":pCpu->name);

                            bNoWorkForCurrentClock = 0;                     /* might be more work */
                        } 
                    } 
                    else /* stage is a sink */
                    if (pStage->StageType == sink)
                    { 
                        pCpu = (pStage->CpuNum == NA)? NULL:&Cpu[pStage->CpuNum]; 

                        /* consume a new frame? */
                        if ( pFrame->TimeLeftInStage == NA &&
                            (pStage->InputFramesCurrent != 0) &&
                            (pCpu==NULL || pCpu->CurrentStage == NA))                        
                        { 
                            pStage->InputFramesCurrent--;                  /* decrement available input frames */
                            if (pCpu) pCpu->CurrentStage = pStage->StageNum;    /* grab cpu for this stage */

                            /* start the clock */
                            if (pFrame->FrameNum >0){
                               pFrame->TimeLeftInStage = (Path[i].Frame[0].EndTime + 
                                    (1000*j)/Path[i].Fps) - Clock;
                            } 
                            if (pFrame->FrameNum ==0  || pStage->AvgTime>pFrame->TimeLeftInStage) { 
                                pFrame->TimeLeftInStage = pStage->AvgTime;
                            } 
                            pFrame->FrameStage = pStage->StageNum;

                            printf("%05i:%s Frame %i started consuming in %s (%s)\n", Clock, Path[i].name, 
                                pFrame->FrameNum, pStage->name, (pCpu==NULL) ? "":pCpu->name);

                            bNoWorkForCurrentClock = 0;                     /* might be more work */            
                        } 

                        /* finish a frame? */
                        if (pFrame->TimeLeftInStage == 0)
                        { 
                            pPrevStage = &Stage[Path[i].Stage[pStage->StageNum - 1]];
                            pPrevStage->OutputFramesCurrent++;
                            pFrame->FrameStage = NA;                        
                            pFrame->TimeLeftInStage = NA;                   /* not being processed so no time left for now */
                            pCpu->CurrentStage = NA;                       /* relinquish cpu */

                            Path[i].FramesDone++;
                            pFrame->EndTime = Clock;

                            printf("%05i:%s Frame %i done processing in %s (%s)\n", Clock, Path[i].name, 
                                pFrame->FrameNum, pStage->name, (pCpu==NULL) ? "":pCpu->name);

                            bNoWorkForCurrentClock = 0;                     /* might be more work */
                        } 
                    } 
                } 
            } 
        }  
        while (!bNoWorkForCurrentClock);

        /* see if all frames for all paths are done */
        bNoWorkAtAll = 1;
        for (i=0; i<PathsTotal; i++)
        { 
            if (Path[i].FramesDone < Path[i].TotalFrames)
                bNoWorkAtAll = 0;
        } 
     
        /* decrement time left on all frames being processed */
        for (i=0;i<PathsTotal; i++)
        { 
            for (j=Path[i].FramesDone; j<Path[i].CurrentFrames;j++)
            { 
                pFrame = &Path[i].Frame[j];            
                if ((pFrame->TimeLeftInStage != NA) 
                    && (pFrame->TimeLeftInStage != 0))
                { 
                    pFrame->TimeLeftInStage--;
                } 
            } 
        }

        /* increment clock */
        Clock++;

    }  
    while(!bNoWorkAtAll);

    for (i=0;i<PathsTotal; i++)
    { 
        printf("\n\n%s Path:\n", Path[i].name);    
        for (j=0; j<Path[i].TotalFrames;j++)
        { 
            printf("Frame %03i: start=%05i end=%05i expected_end=%05i\n",
                j, Path[i].Frame[j].StartTime, Path[i].Frame[j].EndTime,
                Path[i].Frame[0].EndTime + (j*1000)/Path[i].Fps);
        }
    }

    return 0;
}



