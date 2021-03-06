//-------------------------------------------------
procedure CheckCommandLine;
const
  cClassA = 'TFormViewUV';
  cClassW = cClassA + '.UnicodeClass';
var  
  S, SParam: WideString;
  Wnd: THandle;
  Data: TCopyDataStruct;
  AClassName: string;
  ASingleInst: boolean;
begin
  S:= SParamStrW(1);

  //Check /Opt param
  if UpperCase(S) = '/OPT' then
    begin
    OpenUniversalViewerOptions;
    Halt;
    end;

  //Don't check control params
  if SBegin(S, '/') then
    S:= '';

  if S <> '' then
    begin
    SParam:= S;

    //Handle @@Folder parameter
    if SBegin(SParam, '@@') then
      Delete(SParam, 1, 2)
    else
    //Handle @Filelist parameter
    if SBegin(SParam, '@') then
      Delete(SParam, 1, 1);

    //Check filename existing (don't check folders)
    if not IsDirExist(SParam) then
      begin
      if not IsFileExist(SParam) then
        begin
        MsgError(SFormatW(MsgViewerErrCannotFindFile, [SParam]));
        Halt
        end;

      if not IsFileAccessed(SParam) then
        begin
        MsgError(SFormatW(MsgViewerErrCannotOpenFile, [SParam]));
        Halt
        end;
      end;
    end;

  //Handle "Single instance" option
  with TIniFile.Create(ConfigFolder + '\Viewer.ini') do
  try
    ASingleInst:= ReadBool(csOpt, ccOSingleInst, false);
  finally
    Free;
  end;

  if ASingleInst then
    begin
    if Win32Platform = VER_PLATFORM_WIN32_NT then
      AClassName:= cClassW
    else
      AClassName:= cClassA;

    Wnd:= FindWindow(PChar(AClassName), nil);
    if Wnd <> 0 then
      begin
      FillChar(Data, SizeOf(Data), 0);
      Data.dwData:= 101; //"Open Unicode filename"
      if S <> '' then
        begin
        Data.cbData:= (Length(S) + 1) * 2;
        Data.lpData:= PWChar(S);
        end;
      SendMessage(Wnd, WM_COPYDATA, 0{hWnd}, integer(@Data));
      SetForegroundWindow(Wnd);
      Halt;
      end;
    end;
end;

//-------------------------------------------------
function TFormViewUV.ReadCommandLine: WideString;
  //
  function SGetListValue(var S: WideString): WideString;
  var
    i: integer;
  begin
    i:= Pos(WideString(','), S);
    if i = 0 then i:= MaxInt;
    Result:= Copy(S, 1, i-1);
    Delete(S, 1, i);
  end;
  //
var
  i: integer;
  S, SUpper: WideString;
begin
  Result:= '';
  for i:= 1 to SParamCount do
    begin
    S:= SParamStrW(i);
    SUpper:= UpperCase(S);

    //Handle control parameters
    if SBegin(S, '/') then
      begin
      // /Q: Start in "Quick View" mode.
      if SUpper = '/Q' then
        begin
        FQuickViewMode:= true;
        //Hide menu 
        ShowMenu:= false;
        EnableMenu:= false;
        //Hide toolbar
        Toolbar.Visible:= false;
        //Hide navigation panel
        {$ifdef NAV}
        ShowNav:= false;
        {$endif}
        //Hide min/max buttons
        BorderIcons:= [biSystemMenu];
        //Set "Always on top" style
        FormStyle:= fsStayOnTop;
        //Disable message boxes
        ATViewerMessagesEnabled:= false;
        Continue;
        end;

      // /QB: Hide window border+caption.
      if SUpper = '/QB' then
        begin
        //BorderStyle:= bsNone;
        SetFormStyle(Self, false);
        Continue;
        end;

      // /QT: Hide taskbar icon.
      if SUpper = '/QT' then
        begin
        FNoTaskbarIcon:= true;
        Continue;
        end;

      // /P: Specify window position.
      if SBegin(SUpper, '/P=') then
        begin
        S:= SVal(S);
        if (S <> '') then Left:= StrToIntDef(SGetListValue(S), Left);
        if (S <> '') then Top:= StrToIntDef(SGetListValue(S), Top);
        if (S <> '') then Width:= StrToIntDef(SGetListValue(S), Width);
        if (S <> '') then Height:= StrToIntDef(SGetListValue(S), Height);
        Continue;
        end;

      // /Dis
      if SBegin(SUpper, '/DIS=') then
        begin
        S:= SVal(S);
        if Pos(',EditCopy,', ','+S+',')>0 then
          Viewer.TextEnableSel:= false;
        while S<>'' do
          FToolbarList.Disable(SGetListValue(S));
        Continue;
        end;

      // /Pos: Specify file position.
      if SBegin(SUpper, '/POS=') then
        begin
        S:= SVal(S);
        if (S <> '') then
          begin
          FStartupPosDo:= true;
          FStartupPosLine:= UpperCase(S[Length(S)]) = 'N';
          FStartupPosPercent:= UpperCase(S[Length(S)]) = 'P';
          if not (Char(S[Length(S)]) in ['0'..'9']) then
            Delete(S, Length(S), 1);
          FStartupPos:= StrToIntDef(S, 0);
          Viewer.WebWaitForNavigate:= true;
          end;
        Continue;
        end;

      // /Mode: Specify view mode.
      if Sbegin(SUpper, '/MODE=') then
        begin
        S:= SVal(S);
        Viewer.ModeDetect:= false;
        Viewer.Mode:= IntegerToMode(StrToIntDef(S, 1), vmodeText);
        Continue;
        end;

      //Search params
      if Sbegin(SUpper, '/FTEXT=') then
        begin
        FFindText:= SVal(S);
        Continue;
        end;

      if Sbegin(SUpper, '/FCASE=') then
        begin
        FFindCase:= SVal(S) = '1';
        Continue;
        end;

      if Sbegin(SUpper, '/FHEX=') then
        begin
        FFindHex:= SVal(S) = '1';
        Continue;
        end;

      if Sbegin(SUpper, '/FWORDS=') then
        begin
        FFindWords:= SVal(S) = '1';
        Continue;
        end;

      // /Print: print and exit.
      if SUpper = '/PRINT' then
        begin
        FStartupPrint:= true;
        Continue;
        end;
      end;

    //Handle @@Folder parameter
    if SBegin(S, '@@') then
      begin
      Delete(S, 1, 2);
      if FFileList.ReadDirectory(S) then
        begin
        if FFileList.Count = 0 then
          begin
          MsgError(SFormatW(MsgViewerDirEmpty, [S]));
          S:= '';
          Halt;
          end
        else
          begin
          FFileList.Locked:= true;
          FFileList.LockedDir:= true;
          S:= FFileList.GetFirst;
          end;
        end
      else
        Break;
      end;

    //Handle @Filelist parameter
    if Sbegin(S, '@') then
      begin
      Delete(S, 1, 1);
      if FFileList.ReadFileList(S) then
        S:= FFileList.GetFirst
      else
        Break;
      end;

    //Filename (maybe got from @Filelist) returned as result
    if not SBegin(S, '/') then
      Result:= S;
    end;
end;

//-------------------------------------------------
{$ifdef NAV}
procedure TFormViewUV.WMCopyData(var Msg: TWMCopyData);
var
  S: WideString;
begin
  BOOL(Msg.Result):= Self.Visible;
  if BOOL(Msg.Result) then
    begin
    S:= '';
    with Msg.CopyDataStruct^ do
      case dwData of
        100: //"Open ANSI filename"
          S:= AnsiString(PAnsiChar(lpData));
        101: //"Open Unicode filename"
          S:= WideString(PWideChar(lpData));
      end;
    if S = '' then Exit;

    Application.Restore;
    FFileList.Locked:= false; //Clear filelist

    //Handle @@Folder
    if Sbegin(S, '@@') then
      begin
      Delete(S, 1, 2);
      if FFileList.ReadDirectory(S) then
        begin
        if FFileList.Count = 0 then
          begin
          MsgWarning(SFormatW(MsgViewerDirEmpty, [S]));
          S:= '';
          end
        else
          begin
          FFileList.Locked:= true;
          FFileList.LockedDir:= true;
          S:= FFileList.GetFirst;
          end
        end
      else
        S:= '';
      end;

    //Handle @FileList
    if SBegin(S, '@') then
      begin
      Delete(S, 1, 1);
      if FFileList.ReadFileList(S) then
        S:= FFileList.GetFirst
      else
        S:= '';
      end;

    //Both files and folders allowed
    if IsFileOrDirExist(S) then
      begin
      FFileNameWM:= S;
      TimerWM.Enabled:= true;
      end
    else
      CloseFile;
    end;
end;
{$endif}

