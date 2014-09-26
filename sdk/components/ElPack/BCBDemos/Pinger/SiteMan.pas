unit SiteMan;

interface

uses ElGraphs, ExtCtrls, ElPgCtl, Classes, Forms, SysUtils, ElTimers;

type TSite = class
       private
         FInterval: integer;
         procedure SetInterval(Value : integer);
         procedure SetRepCount(Value : integer);
         //procedure SetRepFile (Value : string);
       public
        Shutdown : boolean;
        Address  : string;
        HostName : string;
        Timeout  : integer;
        Graph : TElGraph;
        Sheet : TElTabSheet;
        StartTime : string;
        ThreadList : TList;
        Timer      : TElTimerPoolItem;
        FRepFile : string;

        F : TextFile;
        FRepCount,
        RepNow : integer;
        procedure SaveValue;
        procedure Start;
        destructor Destroy; override;
        procedure TimerProc(Sender:TObject);
        procedure OnThreadFinish(Sender:TObject);

        property RepFile  : string read FRepFile write FRepFile;
        property RepCount : integer read FRepCount write SetRepCount;
        property Interval : integer read FInterval write SetInterval;
     end;

implementation

uses PingThread, Main;

procedure TSite.SetRepCount(Value : integer);
begin
  if (FRepFile <> '') and (Value >0) and (Value <>FRepCount) then
  begin
    if (FRepCount = 0) then
    begin
      try
        if not FileExists(FRepFile) then
        begin
          AssignFile(F, FRepFile);
          Rewrite(F);
        end else
        begin
          AssignFile(F, FRepFile);
          Append(F);
        end;
        Writeln(F, '');
        Write(F, 'Time: ', DateTimeToStr(Now));
        WriteLn(F, ' EldoS Pinger started logging.');
        CloseFile(F);
      except
      end;
     end;
  end;
  FRepCount := Value;
end;

procedure TSite.SetInterval;
var Event : TElTimerPoolItem;
begin
  if (Value <> FInterval) and (Value >=500) then
  begin
    FInterval := Value;
    Event := Timer;
    Event.Interval := Value;
  end;
end;

procedure TSite.Start;
begin
  ThreadList:=TList.Create;
  if Interval = 0 then
    FInterval := 1000;
  Timer := MainForm.TimerList.Items.Add;
  Timer.Interval := FInterval;
  Timer.OnTimer := TimerProc;
  Timer.Enabled := true;
end;
                 
destructor TSite.Destroy;
begin
  Timer.Free;

  while ThreadList.Count >0 do
    Application.ProcessMessages;

  ThreadList.Free;
  inherited;
end;

procedure TSite.OnThreadFinish(Sender:TObject);
begin
  ThreadList.Remove(Sender);
end;

procedure TSite.TimerProc;
var Thread : TPingThread;
begin
  if Shutdown then exit;
  Thread := TPingThread.Create(true);
  Thread.Address:=Address;
  Thread.Graph:=Graph;
  Thread.Site := Self;
  Thread.Timeout := Timeout;
  Thread.FreeOnTerminate:=true;
  Thread.OnTerminate:=OnThreadFinish;
  ThreadList.Add(Thread);
  Thread.Resume;
end;

procedure TSite.SaveValue;  { public }
var Min,
    Max,
    Avg : integer;
begin
  if (RepCount = 0) or (RepFile = '') then exit;
  inc (RepNow);
  if RepNow >= RepCount then
  begin
    RepNow := 0;
    try
      AssignFile(F, RepFile);
      Append(F);
      Write(F, 'Time: ', DateTimeToStr(Now));
      Write(F, ' Faults: ', Graph.DataList[0].Faults);
      Graph.DataList[0].CalcMinMax(Min, Max, Avg);
      Write(F, ' Average: ', Avg);
      WriteLn(F, ' Last: ', Graph.DataList[0].Value[Graph.DataList[0].ValueCount-1]);

      CloseFile(F);
    except
      on E: Exception do ;
    end;
  end;
end;  { SaveValue }

end.
