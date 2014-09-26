unit PingThread;

interface

uses
  Windows, Classes, ElGraphs, ICMP, SiteMan, SysUtils;

type
  TPingThread = class(TThread)
  private
    res : integer;
  protected
    procedure Execute; override;
    procedure Report;
  public
    Address : string;
    Timeout  : integer;
    DataSize : integer;
    Graph   : TElGraph;
    Site : TSite;

    property Terminated;
  end;

implementation

{ TPingThread }

procedure TPingThread.Report;
begin
  Graph.DataList[0].AddValue(res);
  Site.SaveValue;
  Graph.Repaint;        
end;

procedure TPingThread.Execute;
var xPing: TICMP;
begin
  xPing := nil;
  try
   try
    xPing:=TICMP.Create;
    xPing.Address := Address;
    xPing.Timeout := Timeout;    
    xPing.Size := DataSize;
    res:=xPing.Ping;
    if (res<>0) then res := xPing.Reply.RTT else res:=-1;
    Synchronize(report);
   finally
    xPing.Free;
   end;
  except
    on E : Exception do ;
  end;
end;

end.
