// SPDX-License-Identifier: GPL-2.0-or-later
// Ordinary .NET Framework desktop application. No packer, extraction, network,
// global keyboard hook, process injection, installation or administrator rights.
using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.IO.Ports;
using System.Management;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using System.Web.Script.Serialization;
using System.Windows.Forms;

[assembly: AssemblyTitle("Pocket World OLED Controller")]
[assembly: AssemblyDescription("Local USB controller for ESP32-C3 OLED games")]
[assembly: AssemblyProduct("Pocket World")]
[assembly: AssemblyCompany("Pocket World Project")]
[assembly: AssemblyVersion("0.2.1.0")]
[assembly: AssemblyFileVersion("0.2.1.0")]

namespace PocketWorld {
    sealed class OledView : Control {
        public byte[] Pixels;
        public bool Readback=true;
        public OledView() { DoubleBuffered=true; BackColor=Color.FromArgb(3,6,8); TabStop=true; SetStyle(ControlStyles.Selectable,true); }
        protected override bool IsInputKey(Keys k) { return true; }
        protected override void OnMouseDown(MouseEventArgs e) { Focus(); base.OnMouseDown(e); }
        protected override void OnPaint(PaintEventArgs e) {
            base.OnPaint(e);
            int s=Math.Max(1,Math.Min(Width/72,Height/40)),ox=(Width-s*72)/2,oy=(Height-s*40)/2;
            if(!Readback||Pixels==null) { TextRenderer.DrawText(e.Graphics,Readback?"ESP32 연결 대기":"라이브 뷰 꺼짐 · OLED에서 실행 중",Font,ClientRectangle,Color.Silver,TextFormatFlags.HorizontalCenter|TextFormatFlags.VerticalCenter); return; }
            using(var b=new SolidBrush(Color.FromArgb(220,246,233)))
                for(int y=0;y<40;y++) for(int x=0;x<72;x++) if((Pixels[y/8*72+x]&(1<<(y%8)))!=0) e.Graphics.FillRectangle(b,ox+x*s,oy+y*s,s,s);
        }
    }

    sealed class Controller : Form {
        static readonly Color Bg=Color.FromArgb(17,24,32),PanelBg=Color.FromArgb(26,37,48),Fg=Color.FromArgb(233,242,237),Muted=Color.FromArgb(145,167,177),Accent=Color.FromArgb(168,231,160);
        readonly OledView view=new OledView();
        readonly Label connection=new Label(),status=new Label(),metrics=new Label(),lockStatus=new Label();
        readonly TextBox log=new TextBox();
        readonly ComboBox stage=new ComboBox(),portChoice=new ComboBox();
        readonly CheckBox clean=new CheckBox();
        readonly CheckBox live=new CheckBox();
        readonly List<Button> modes=new List<Button>();
        readonly HashSet<Keys> keys=new HashSet<Keys>();
        readonly JavaScriptSerializer json=new JavaScriptSerializer();
        readonly Timer timer=new Timer();
        readonly StringBuilder incoming=new StringBuilder();
        readonly string verifyPath;
        SerialPort serial;
        bool connecting,ready,locked,previewAuto,wtsRegistered,verifying;
        volatile bool closing;
        int mode=2,gamma=50,dither=15,light=150,receivedFrames,wireFrames;
        long nextConnect,lastHello,lastKey,lastState;
        string requestedPort="자동";
        Dictionary<string,object> boardState;
        [DllImport("wtsapi32.dll",SetLastError=true)] static extern bool WTSRegisterSessionNotification(IntPtr h,int flags);
        [DllImport("wtsapi32.dll")] static extern bool WTSUnRegisterSessionNotification(IntPtr h);
        [DllImport("user32.dll")] static extern IntPtr SendMessage(IntPtr h,int msg,IntPtr w,IntPtr l);
        static long Now { get { return DateTime.UtcNow.Ticks/TimeSpan.TicksPerMillisecond; } }
        public Controller(string verify) {
            verifyPath=verify; Text="Pocket World · ESP32 OLED Controller"; BackColor=Bg; ForeColor=Fg;
            Font=new Font("Malgun Gothic",10); AutoScaleMode=AutoScaleMode.Dpi;
            ClientSize=new Size(1120,810); MinimumSize=new Size(1000,780); StartPosition=FormStartPosition.CenterScreen; KeyPreview=true;
            BuildUi(); Deactivate+=(s,e)=>ClearKeys();
            timer.Interval=25; timer.Tick+=(s,e)=>Tick(); timer.Start();
            Shown+=(s,e)=>view.Focus();
        }
        Label Label(string text,int size,Color color) { return new Label {Text=text,Font=new Font(Font.FontFamily,size),ForeColor=color,AutoSize=true,Margin=new Padding(0,7,0,5)}; }
        Button Button(string text,Action click) {
            var b=new Button {Text=text,AutoSize=false,Width=254,Height=36,FlatStyle=FlatStyle.Flat,BackColor=Color.FromArgb(40,58,73),ForeColor=Fg,Margin=new Padding(0,4,0,4),TabStop=false};
            b.FlatAppearance.BorderSize=0; b.Click+=(s,e)=>{click();view.Focus();};return b;
        }
        void StyleLabel(Label l,string text,Color color) { l.Text=text;l.ForeColor=color;l.AutoSize=true;l.Margin=new Padding(0,8,0,4); }
        void BuildUi() {
            var root=new TableLayoutPanel {Dock=DockStyle.Fill,Padding=new Padding(26,20,26,20),ColumnCount=1,RowCount=3};
            root.RowStyles.Add(new RowStyle(SizeType.Absolute,52)); root.RowStyles.Add(new RowStyle(SizeType.Absolute,32)); root.RowStyles.Add(new RowStyle(SizeType.Percent,100));Controls.Add(root);
            var head=new TableLayoutPanel {Dock=DockStyle.Fill,ColumnCount=2};head.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,55));head.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,45));
            head.Controls.Add(Label("POCKET WORLD",22,Fg),0,0);StyleLabel(connection,"● 보드 연결 대기",Muted);connection.Anchor=AnchorStyles.Right;head.Controls.Add(connection,1,0);root.Controls.Add(head,0,0);
            root.Controls.Add(Label("작은 세계는 보드 안에서. PC에서는 선택하고, 조작하세요.",10,Muted),0,1);
            var content=new TableLayoutPanel {Dock=DockStyle.Fill,ColumnCount=2};content.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,100));content.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute,280));root.Controls.Add(content,0,2);
            var left=new TableLayoutPanel {Dock=DockStyle.Fill,Padding=new Padding(18),BackColor=PanelBg,ColumnCount=1,RowCount=7,Margin=new Padding(0,10,18,0)};
            left.RowStyles.Add(new RowStyle(SizeType.Absolute,34));left.RowStyles.Add(new RowStyle(SizeType.Percent,100));left.RowStyles.Add(new RowStyle(SizeType.Absolute,34));left.RowStyles.Add(new RowStyle(SizeType.Absolute,32));left.RowStyles.Add(new RowStyle(SizeType.Absolute,34));left.RowStyles.Add(new RowStyle(SizeType.Absolute,54));left.RowStyles.Add(new RowStyle(SizeType.Absolute,95));
            var previewHeader=new TableLayoutPanel {Dock=DockStyle.Fill,ColumnCount=2,Margin=new Padding(0)};
            previewHeader.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,100));previewHeader.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute,120));
            previewHeader.Controls.Add(Label("LIVE OLED    72 × 40 · 1 BIT",11,Fg),0,0);
            live.Text="라이브 뷰";live.Checked=true;live.AutoSize=true;live.Anchor=AnchorStyles.Right;
            live.CheckedChanged+=(s,e)=>{view.Readback=live.Checked;view.Pixels=null;Send("MIRROR "+(live.Checked?1:0));view.Invalidate();view.Focus();};
            previewHeader.Controls.Add(live,1,0);left.Controls.Add(previewHeader,0,0);view.Dock=DockStyle.Fill;left.Controls.Add(view,0,1);
            StyleLabel(status,"DOOM · 원작 플레이",Accent);left.Controls.Add(status,0,2);StyleLabel(metrics,"체력 —    탄약 —    맵 —",Muted);left.Controls.Add(metrics,0,3);
            left.Controls.Add(Label("방향키 이동/회전   Ctrl 공격   Space 문/확인",10,Fg),0,4);
            left.Controls.Add(Label("Q/E 옆걸음   Enter 메뉴   Tab 지도\n화면을 클릭한 뒤 조작하세요. 다른 창에서는 입력하지 않습니다.",9,Muted),0,5);
            log.Multiline=true;log.ReadOnly=true;log.Dock=DockStyle.Fill;log.BackColor=Bg;log.ForeColor=Muted;log.BorderStyle=BorderStyle.None;log.ScrollBars=ScrollBars.Vertical;left.Controls.Add(log,0,6);content.Controls.Add(left,0,0);
            var right=new FlowLayoutPanel {Dock=DockStyle.Fill,FlowDirection=FlowDirection.TopDown,WrapContents=false,AutoScroll=true,Padding=new Padding(7,4,0,0)};content.Controls.Add(right,1,0);
            right.Controls.Add(Label("화면과 게임",12,Fg));
            string[] names={"펫 · 함께 생활하기","던전 · 탐험하기","DOOM · 원작 플레이"};
            for(int i=0;i<3;i++) {int m=i;var b=Button(names[i],()=>SelectMode(m));modes.Add(b);right.Controls.Add(b);} PaintMode();
            var row=new FlowLayoutPanel {Width=254,Height=42,Margin=new Padding(0,8,0,4)};
            stage.DropDownStyle=ComboBoxStyle.DropDownList;stage.ForeColor=Bg;stage.BackColor=Fg;stage.Width=92;for(int i=1;i<=9;i++)stage.Items.Add("E1M"+i);stage.SelectedIndex=0;row.Controls.Add(stage);
            var start=Button("새 게임",()=>{SelectMode(2);Send("NEW "+(stage.SelectedIndex+1));});start.Width=150;start.Margin=new Padding(4,0,0,0);row.Controls.Add(start);right.Controls.Add(row);
            var care=new FlowLayoutPanel {Width=254,Height=42,Margin=new Padding(0,0,0,0)};
            string[] labels={"먹이","놀이","휴식"},commands={"FEED","PLAY","REST"};for(int i=0;i<3;i++){string c=commands[i];var b=Button(labels[i],()=>Send("PET "+c));b.Width=78;b.Margin=new Padding(0,2,4,2);care.Controls.Add(b);}right.Controls.Add(care);
            StyleLabel(lockStatus,"Windows 잠금 감지 준비",Accent);right.Controls.Add(lockStatus);
            right.Controls.Add(Button("자동 생활 미리보기",()=>{previewAuto=!previewAuto;ApplyLock();}));
            clean.Text="DOOM 가시성 향상";clean.Checked=true;clean.AutoSize=true;clean.Margin=new Padding(0,8,0,2);clean.CheckedChanged+=(s,e)=>{Send("CLEAN "+(clean.Checked?1:0));view.Focus();};right.Controls.Add(clean);
            Slider(right,"OLED 밝기",20,220,light,v=>{light=v;Send("LIGHT "+v);});
            Slider(right,"어두운 장면 보정",0,100,gamma,v=>{gamma=v;Send("GAMMA "+v);});
            Slider(right,"디더링 강도",0,100,dither,v=>{dither=v;Send("DITHER "+v);});
            right.Controls.Add(Label("USB 연결",9,Muted));portChoice.DropDownStyle=ComboBoxStyle.DropDownList;portChoice.ForeColor=Bg;portChoice.BackColor=Fg;portChoice.Width=254;portChoice.Items.Add("자동");foreach(string p in SerialPort.GetPortNames())portChoice.Items.Add(p);portChoice.SelectedIndex=0;
            portChoice.SelectedIndexChanged+=(s,e)=>{requestedPort=portChoice.Text;Disconnect();nextConnect=0;view.Focus();};right.Controls.Add(portChoice);
            right.Controls.Add(Label(".NET 기본 앱 · USB 통신만 사용",8,Muted));
            Log("ESP32를 자동으로 연결합니다. 게임은 보드 안에서 실행됩니다.");
        }
        void Slider(FlowLayoutPanel p,string title,int min,int max,int value,Action<int> changed) {
            p.Controls.Add(Label(title,9,Muted));var t=new TrackBar {Minimum=min,Maximum=max,Value=value,Width=254,Height=28,AutoSize=false,TickStyle=TickStyle.None,Margin=new Padding(0)};t.ValueChanged+=(s,e)=>changed(t.Value);p.Controls.Add(t);
        }
        protected override void OnHandleCreated(EventArgs e) { base.OnHandleCreated(e);wtsRegistered=WTSRegisterSessionNotification(Handle,0);lockStatus.Text=wtsRegistered?"Windows 잠금 감지 활성":"잠금 감지 등록 실패"; }
        protected override void WndProc(ref Message m) { if(m.Msg==0x2b1 && (m.WParam.ToInt32()==7||m.WParam.ToInt32()==8)){locked=m.WParam.ToInt32()==7;ApplyLock();}base.WndProc(ref m); }
        protected override void OnHandleDestroyed(EventArgs e) {if(wtsRegistered)WTSUnRegisterSessionNotification(Handle);base.OnHandleDestroyed(e);}
        void ApplyLock() {ClearKeys();Send("LOCK "+((locked||previewAuto)?1:0));lockStatus.Text=locked||previewAuto?"자동 생활 중":"Windows 잠금 감지 활성";}
        static int KeyBit(Keys k) { switch(k) {
            case Keys.Space:return 0;case Keys.ControlKey:case Keys.LControlKey:case Keys.RControlKey:case Keys.Z:return 1;
            case Keys.Q:return 2;case Keys.E:return 3;case Keys.Up:case Keys.W:return 4;case Keys.Down:case Keys.S:return 5;
            case Keys.Left:case Keys.A:return 6;case Keys.Right:case Keys.D:return 7;case Keys.Enter:case Keys.Escape:return 8;case Keys.Tab:return 9;
            default:return -1;
        } }
        bool GameFocus { get {return ActiveForm==this&&!stage.Focused&&!portChoice.Focused&&!log.Focused;} }
        protected override bool ProcessCmdKey(ref Message msg,Keys keyData) {
            Keys k=keyData&Keys.KeyCode;if(GameFocus&&KeyBit(k)>=0){keys.Add(k);if((keyData&Keys.Control)!=0)keys.Add(Keys.ControlKey);return true;}return base.ProcessCmdKey(ref msg,keyData);
        }
        protected override void OnKeyDown(KeyEventArgs e) {if(GameFocus&&KeyBit(e.KeyCode)>=0){keys.Add(e.KeyCode);e.Handled=true;}base.OnKeyDown(e);}
        protected override void OnKeyUp(KeyEventArgs e) {keys.Remove(e.KeyCode);if(e.KeyCode==Keys.ControlKey){keys.Remove(Keys.LControlKey);keys.Remove(Keys.RControlKey);}base.OnKeyUp(e);}
        int KeyMask() {int n=0;if(!GameFocus||locked||previewAuto)return 0;foreach(Keys k in keys){int bit=KeyBit(k);if(bit>=0)n|=1<<bit;}return n;}
        void ClearKeys() {keys.Clear();Send("KEY 0");}
        void PaintMode() {for(int i=0;i<modes.Count;i++){modes[i].BackColor=i==mode?Accent:Color.FromArgb(40,58,73);modes[i].ForeColor=i==mode?Bg:Fg;}}
        void SelectMode(int m) {ClearKeys();mode=m;PaintMode();Send("MODE "+m);}
        void Log(string s) {if(log.Lines.Length>30)log.Clear();log.AppendText(DateTime.Now.ToString("HH:mm:ss")+"  "+s+Environment.NewLine);}
        void Send(string s) {if(!ready||serial==null)return;try{serial.Write(s+"\n");}catch(IOException){Disconnect();}catch(InvalidOperationException){Disconnect();}catch(TimeoutException){Disconnect();}}
        void Disconnect() {ready=false;keys.Clear();incoming.Clear();if(serial!=null){try{serial.Dispose();}catch{}serial=null;}connection.Text="● 다시 연결하는 중";connection.ForeColor=Muted;nextConnect=Now+2000;}
        static string FindPort(string selected) {
            if(selected!="자동")return selected;
            using(var search=new ManagementObjectSearcher("SELECT Name,PNPDeviceID FROM Win32_PnPEntity WHERE PNPDeviceID LIKE '%VID_303A%PID_1001%'"))
                foreach(ManagementObject p in search.Get()){var m=Regex.Match(Convert.ToString(p["Name"]),@"\((COM\d+)\)");if(m.Success)return m.Groups[1].Value;}
            return null;
        }
        void Connect() {
            connecting=true;string selection=requestedPort;
            Task.Run(()=>{
                SerialPort p=null;string error=null;
                try {string name=FindPort(selection);if(name!=null){p=new SerialPort(name,115200);p.ReadTimeout=20;p.WriteTimeout=150;p.DtrEnable=true;p.RtsEnable=false;p.Open();}}
                catch(Exception ex){error=ex.Message;if(p!=null)p.Dispose();p=null;}
                if(closing){if(p!=null)p.Dispose();return;}
                try {BeginInvoke((Action)(()=>{connecting=false;nextConnect=Now+2500;if(closing){if(p!=null)p.Dispose();return;}if(p!=null){serial=p;incoming.Clear();lastHello=0;Log(p.PortName+" USB 연결 · 응답 대기");}else if(error!=null)Log(error);}));}
                catch(InvalidOperationException){if(p!=null)p.Dispose();}
            });
        }
        void Tick() {
            if(closing)return;
            if(serial==null){if(!connecting&&Now>=nextConnect)Connect();return;}
            try {
                if(!ready&&Now-lastHello>750){serial.Write("HELLO\n");lastHello=Now;}
                if(serial.BytesToRead>0)incoming.Append(serial.ReadExisting());
                for(int count=0;count<100;count++) {string all=incoming.ToString();int i=all.IndexOf('\n');if(i<0)break;string line=all.Substring(0,i).Trim();incoming.Remove(0,i+1);Parse(line);}
                if(incoming.Length>32768)incoming.Clear();
                if(ready&&Now-lastKey>75){Send("KEY "+KeyMask());lastKey=Now;}
                if(ready&&lastState>0&&Now-lastState>5000){Disconnect();return;}
                if(verifyPath!=null&&!verifying&&receivedFrames>=15&&boardState!=null)Verify();
            } catch(IOException){Disconnect();}catch(InvalidOperationException){Disconnect();}catch(TimeoutException){Disconnect();}
        }
        static byte[] DecodeHex(string h) {if(h.Length!=720)return null;var b=new byte[360];for(int i=0;i<360;i++)b[i]=Convert.ToByte(h.Substring(i*2,2),16);return b;}
        void Parse(string line) {
            Dictionary<string,object> m;try{m=json.Deserialize<Dictionary<string,object>>(line);}catch{if(line.StartsWith("DOOM ERROR"))Log(line);return;}
            if(m==null||!m.ContainsKey("t"))return;
            switch(Convert.ToString(m["t"])) {
                case "hello":if(!m.ContainsKey("id")||Convert.ToString(m["id"])!="oled-doom"||!m.ContainsKey("v")||Convert.ToInt32(m["v"])!=1)return;
                    ready=true;lastState=Now;connection.Text="● "+serial.PortName+" 연결됨";connection.ForeColor=Accent;
                    Send("MODE "+mode);ApplyLock();Send("MIRROR "+(live.Checked?1:0));Send("LIGHT "+light);Send("GAMMA "+gamma);Send("DITHER "+dither);Send("CLEAN "+(clean.Checked?1:0));Log("보드 연결 완료 · 네이티브 실행");break;
                case "frame":wireFrames++;try{if(!live.Checked)break;var b=DecodeHex(Convert.ToString(m["hex"]));if(b!=null){view.Pixels=b;receivedFrames++;view.Invalidate();}}catch(FormatException){}break;
                case "state":boardState=m;lastState=Now;status.Text=Convert.ToBoolean(m["auto"])?"자동 생활 중":new[]{"펫 · 생활","던전 · 탐험","DOOM · 원작 플레이"}[Math.Max(0,Math.Min(2,Convert.ToInt32(m["mode"])))];
                    metrics.Text=String.Format("체력 {0}    탄약 {1}    E1M{2}    누적 {3} 프레임",m["hp"],m["ammo"],m["map"],m["frames"]);if(Convert.ToBoolean(m["fault"]))status.Text="보드 오류 · 로그 확인";break;
            }
        }
        async void Verify() {
            verifying=true;
            Directory.CreateDirectory(Path.GetDirectoryName(Path.GetFullPath(verifyPath)));
            live.Checked=false;
            await Task.Delay(500); // Drain packets queued before MIRROR 0.
            Send("STATE");await Task.Delay(200);
            int stoppedAt=wireFrames,boardBefore=Convert.ToInt32(boardState["frames"]);
            using(var shot=new Bitmap(Width,Height)){DrawToBitmap(shot,new Rectangle(0,0,Width,Height));shot.Save(Path.ChangeExtension(verifyPath,"off.png"));}
            await Task.Delay(1500);Send("STATE");await Task.Delay(200);
            bool mirrorStopped=wireFrames==stoppedAt,gameContinued=Convert.ToInt32(boardState["frames"])>boardBefore;
            live.Checked=true;
            await Task.Delay(1000);
            bool mirrorResumed=wireFrames>stoppedAt&&view.Pixels!=null;
            SendMessage(Handle,0x2b1,new IntPtr(7),IntPtr.Zero);bool lockOk=locked;
            SendMessage(Handle,0x2b1,new IntPtr(8),IntPtr.Zero);bool unlockOk=!locked;
            using(var shot=new Bitmap(Width,Height)){DrawToBitmap(shot,new Rectangle(0,0,Width,Height));shot.Save(Path.ChangeExtension(verifyPath,"png"));}
            File.WriteAllText(verifyPath,json.Serialize(new {connected=ready,wts_registered=wtsRegistered,synthetic_lock=lockOk,synthetic_unlock=unlockOk,mirror_stopped=mirrorStopped,game_continued=gameContinued,mirror_resumed=mirrorResumed,frames=receivedFrames,state=boardState}),Encoding.UTF8);
            Close();
        }
        protected override void OnFormClosing(FormClosingEventArgs e) {closing=true;timer.Stop();Send("KEY 0");Send("LOCK 1");Disconnect();base.OnFormClosing(e);}
    }
    static class Program {
        [STAThread] static void Main(string[] args) {
            Application.EnableVisualStyles();Application.SetCompatibleTextRenderingDefault(false);
            try {Application.Run(new Controller(args.Length==2&&args[0]=="--verify"?args[1]:null));}
            catch(Exception e){MessageBox.Show(e.Message,"Pocket World",MessageBoxButtons.OK,MessageBoxIcon.Error);}
        }
    }
}
