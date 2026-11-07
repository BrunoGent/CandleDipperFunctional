import React, { useState, useEffect, useRef } from 'react';
import JSZip from 'jszip';
import { 
  Code2, 
  Layers, 
  Cpu, 
  CheckCircle2, 
  Copy, 
  Check, 
  Eye, 
  Sparkles, 
  Maximize2,
  Sliders,
  RotateCcw,
  Palette,
  Wifi,
  Scale,
  Settings,
  Download,
  FolderArchive,
  Terminal,
  ExternalLink,
  FileCode,
  ShieldCheck,
  CheckCircle,
  Package
} from 'lucide-react';

import mainCppRaw from './main.cpp?raw';
import pinConfigRaw from './PinConfig.h?raw';
import displayManagerHRaw from './DisplayManager.h?raw';
import displayManagerCppRaw from './DisplayManager.cpp?raw';
import motorManagerHRaw from './MotorManager.h?raw';
import motorManagerCppRaw from './MotorManager.cpp?raw';
import scaleManagerHRaw from './ScaleManager.h?raw';
import scaleManagerCppRaw from './ScaleManager.cpp?raw';
import sensorManagerHRaw from './SensorManager.h?raw';
import sensorManagerCppRaw from './SensorManager.cpp?raw';
import soundManagerHRaw from './SoundManager.h?raw';
import soundManagerCppRaw from './SoundManager.cpp?raw';
import platformioIniRaw from '../platformio.ini?raw';
import readmeRaw from '../README.md?raw';
import handoverMdRaw from '../PROJECT_HANDOVER.md?raw';
import userManualRaw from '../USER_MANUAL.md?raw';

export default function App() {
  const [activeTab, setActiveTab] = useState<'export' | 'preview' | 'manual' | 'main' | 'motor' | 'motorcpp' | 'scale' | 'scalecpp' | 'sensor' | 'sensorcpp' | 'sound' | 'soundcpp' | 'header' | 'cpp' | 'pinconfig' | 'platformio' | 'readme' | 'architecture'>('export');
  const [copiedFile, setCopiedFile] = useState<string | null>(null);
  const [isExporting, setIsExporting] = useState<boolean>(false);
  const [exportSuccess, setExportSuccess] = useState<boolean>(false);

  const handleExportZip = async () => {
    setIsExporting(true);
    try {
      const zip = new JSZip();
      
      // Root configuration and documentation
      zip.file("platformio.ini", platformioIniRaw || "");
      zip.file("README.md", readmeRaw || "");
      zip.file("PROJECT_HANDOVER.md", handoverMdRaw || "");
      zip.file("USER_MANUAL.md", userManualRaw || "");
      
      // Complete C++ firmware modules inside src/
      const src = zip.folder("src");
      if (src) {
        src.file("main.cpp", mainCppRaw || "");
        src.file("PinConfig.h", pinConfigRaw || "");
        src.file("DisplayManager.h", displayManagerHRaw || "");
        src.file("DisplayManager.cpp", displayManagerCppRaw || "");
        src.file("MotorManager.h", motorManagerHRaw || "");
        src.file("MotorManager.cpp", motorManagerCppRaw || "");
        src.file("SensorManager.h", sensorManagerHRaw || "");
        src.file("SensorManager.cpp", sensorManagerCppRaw || "");
        src.file("ScaleManager.h", scaleManagerHRaw || "");
        src.file("ScaleManager.cpp", scaleManagerCppRaw || "");
        src.file("SoundManager.h", soundManagerHRaw || "");
        src.file("SoundManager.cpp", soundManagerCppRaw || "");
      }
      
      const content = await zip.generateAsync({ type: "blob" });
      const url = URL.createObjectURL(content);
      const link = document.createElement("a");
      link.href = url;
      link.download = "bee-mine-candle-dipper-antigravity.zip";
      document.body.appendChild(link);
      link.click();
      document.body.removeChild(link);
      URL.revokeObjectURL(url);
      
      setExportSuccess(true);
      setTimeout(() => setExportSuccess(false), 4000);
    } catch (err) {
      console.error("Export zip error", err);
    } finally {
      setIsExporting(false);
    }
  };

  const downloadSingleFile = (filename: string, content: string) => {
    const blob = new Blob([content], { type: "text/plain;charset=utf-8" });
    const url = URL.createObjectURL(blob);
    const link = document.createElement("a");
    link.href = url;
    link.download = filename;
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
    URL.revokeObjectURL(url);
  };

  // M5Stack CoreS3 SE Interactive State (Matches DisplayManager.h AppState)
  const [page, setPage] = useState<number>(0);
  const [scrollOffset, setScrollOffset] = useState<number>(0);
  const [theme, setTheme] = useState<number>(1); // 0: Ind. Slate, 1: Bee Mine, 2: Midnight
  const [brightness, setBrightness] = useState<number>(50);
  const [volume, setVolume] = useState<number>(70);
  const [startupTune, setStartupTune] = useState<number>(1);
  const [finishTune, setFinishTune] = useState<number>(1);
  const [alertTune, setAlertTune] = useState<number>(1);
  const [limitSwitch, setLimitSwitch] = useState<boolean>(false);
  const [capSensor, setCapSensor] = useState<boolean>(false);
  const [weight, setWeight] = useState<number>(0);
  const [scaleFactor, setScaleFactor] = useState<number>(420.0);
  const [calStatus, setCalStatus] = useState<string>('Step 1: Empty jig & press TARE');
  const [motorEnabled, setMotorEnabled] = useState<boolean>(true);
  const [speed, setSpeed] = useState<number>(50);
  const [posMM, setPosMM] = useState<number>(0.0);
  const [lastWaxLevelMM, setLastWaxLevelMM] = useState<number>(360.0);
  const [meltBasesEnabled, setMeltBasesEnabled] = useState<boolean>(true);
  const [isHoming, setIsHoming] = useState<boolean>(false);
  const [isDipBot, setIsDipBot] = useState<boolean>(false);
  const [isMeltingBases, setIsMeltingBases] = useState<boolean>(false);
  const [isCleaningJig, setIsCleaningJig] = useState<boolean>(false);
  const [showNumpad, setShowNumpad] = useState<boolean>(false);
  const [numpadVal, setNumpadVal] = useState<string>('');
  const [activeSettingIdx, setActiveSettingIdx] = useState<number>(0);
  const [lastEvent, setLastEvent] = useState<string>('UI Initialized (2-Bucket Setup + 3-Card Vertical Scroll)');

  const homingTimerRef = useRef<any[]>([]);
  const dipBotTimerRef = useRef<any[]>([]);
  const holdIntervalRef = useRef<any>(null);

  const stopHoming = () => {
    homingTimerRef.current.forEach(clearTimeout);
    homingTimerRef.current = [];
    setIsHoming(false);
    setLastEvent('Homing motion STOPPED by user');
  };

  const stopDipBot = () => {
    dipBotTimerRef.current.forEach(clearTimeout);
    dipBotTimerRef.current = [];
    setIsDipBot(false);
    setIsMeltingBases(false);
    setIsCleaningJig(false);
    setPosMM(0.0);
    setLimitSwitch(true);
    setLastEvent('Motion STOPPED & Retracted to 0.0mm Top Rest');
  };

  const startManualMove = (isUp: boolean) => {
    if (holdIntervalRef.current) clearInterval(holdIntervalRef.current);
    const stepMove = () => {
      setPosMM((prevPos) => {
        if (isUp) {
          if (prevPos <= 0) {
            setLimitSwitch(true);
            return 0;
          }
          const next = Math.max(0, prevPos - (speed * 0.05));
          if (next === 0) setLimitSwitch(true);
          return next;
        } else {
          setLimitSwitch(false);
          return prevPos + (speed * 0.05);
        }
      });
    };
    stepMove();
    holdIntervalRef.current = setInterval(stepMove, 50);
  };

  const stopManualMove = () => {
    if (holdIntervalRef.current) {
      clearInterval(holdIntervalRef.current);
      holdIntervalRef.current = null;
    }
  };

  const settingNames = [
    "Slim Weight", "Std Weight", "Slim Dips", "Std Dips",
    "Dip 1 Time", "Sub Dip Time", "Up Speed", "Down Speed",
    "Col. Limit", "Soft Limit", "Rest Time", "Default Cool",
    "Driver Mode", "Hybrid Speed", "Jig Length", "Melt Time",
    "Clean Time", "Start Cool", "End Cool", "Immerse Spd",
    "Extract Spd", "Drip Dist", "Drip Time", "Known Weight (g)"
  ];
  const settingHints = [
    "Total net wax weight for 36 Slim candles",
    "Total net wax weight for 36 Std candles",
    "Total dip cycles for Slim profile",
    "Total dip cycles for Standard profile",
    "Bottom soak on Dip #1 to saturate wicks",
    "Bottom dwell on Dip #2 and later dips",
    "Fast upward travel in air (above wax)",
    "Fast downward travel in air (to -15mm)",
    "Upward push threshold for auto-retract",
    "Max physical Z travel from top switch",
    "Default top cooling time (if curve=0)",
    "Default cooling pause between dips",
    "Stepper driver current & chopper mode",
    "StealthChop to SpreadCycle threshold",
    "Distance from cap sensor to candle base",
    "Bottom tip soak during Melt Candle Bases",
    "Full-depth soak during Clean Whole Jig",
    "Cooling time after Dip #1 (0 = default)",
    "Cooling time on final dip (0 = default)",
    "Gentle entry speed starting 15mm above",
    "Smooth lift speed while in molten wax",
    "+mm above / -mm in wax (0 = disabled)",
    "Pause duration at Drip Dist offset",
    "Reference mass for 2-point calibration"
  ];
  const settingUnits = [
    "g", "g", "dips", "dips",
    "s", "s", "%", "%",
    "g", "mm", "s", "s",
    "", "mm/s", "mm", "s",
    "s", "s", "s", "%",
    "%", "mm", "s", "g"
  ];
  const [settingValues, setSettingValues] = useState<number[]>([
    1500, 2500, 15, 25, 10, 4, 60, 50, -50, 500, 25, 25, 1, 30, 200, 5, 60, 0, 0, 25, 20, 15, 5, 1000
  ]);
  const [dipAborted, setDipAborted] = useState(false);
  const [collisionAlert, setCollisionAlert] = useState(false);
  const [lowWaxAlert, setLowWaxAlert] = useState(false);
  const [finalDurationSecs, setFinalDurationSecs] = useState(134);
  const [activeIsSlim, setActiveIsSlim] = useState(true);
  const [activeIsWeight, setActiveIsWeight] = useState(true);

  const copyCode = (text: string, filename: string) => {
    navigator.clipboard.writeText(text);
    setCopiedFile(filename);
    setTimeout(() => setCopiedFile(null), 2000);
  };

  // Upgraded Color Palettes for M5Canvas Preview (Matches DisplayManager.cpp applyTheme)
  const getThemeStyles = () => {
    if (theme === 0) { // Theme 0: Industrial Slate & Terracotta ("Ind. Slate")
      return {
        bg: '#0D131A',
        bannerBg: '#0F766E',
        bannerTxt: '#FFFFFF',
        btn1Bg: '#1E407C',
        btn2Bg: '#7F2D3A',
        btnTxt: '#FFFFFF',
        activeBg: '#10B981',
        outlineBg: '#334155'
      };
    } else if (theme === 1) { // Theme 1: Bee Mine Artisanal Honeycomb, Espresso & Botanical Sage
      return {
        bg: '#1C1612',
        bannerBg: '#F5B023',
        bannerTxt: '#000000',
        btn1Bg: '#FCD34D',
        btn2Bg: '#6ECDA5',
        btnTxt: '#000000',
        activeBg: '#4ADE80',
        outlineBg: '#483C32'
      };
    } else { // Theme 2: Midnight Workshop High-Contrast OLED ("Midnight")
      return {
        bg: '#0A0E14',
        bannerBg: '#1E293B',
        bannerTxt: '#E8EDF5',
        btn1Bg: '#253858',
        btn2Bg: '#1F444C',
        btnTxt: '#FFFFFF',
        activeBg: '#22C55E',
        outlineBg: '#404E62'
      };
    }
  };

  const ts = getThemeStyles();

  return (
    <div className="min-h-screen bg-neutral-900 text-neutral-100 flex flex-col font-sans">
      {/* Header Bar */}
      <header className="border-b border-neutral-800 bg-neutral-950 px-6 py-4 flex flex-col md:flex-row md:items-center justify-between gap-4">
        <div className="flex items-center gap-3">
          <div className="w-9 h-9 rounded-lg bg-amber-400/10 border border-amber-400/30 flex items-center justify-center text-amber-400 font-bold text-lg">
            🐝
          </div>
          <div>
            <h1 className="text-lg font-semibold text-white tracking-tight flex items-center gap-2">
              Bee Mine Candle Dipper
              <span className="text-xs px-2 py-0.5 rounded bg-emerald-500/10 border border-emerald-500/30 text-emerald-400 font-mono">
                Antigravity Ready • PC817 Verified
              </span>
            </h1>
            <p className="text-xs text-neutral-400">
              Modular C++ PlatformIO Firmware & Web Telemetry Companion
            </p>
          </div>
        </div>

        {/* Global Export Action Button */}
        <div className="flex items-center gap-3">
          <button
            onClick={handleExportZip}
            disabled={isExporting}
            className="flex items-center gap-2 px-4 py-2 rounded-lg bg-gradient-to-r from-amber-500 to-amber-400 hover:from-amber-400 hover:to-amber-300 text-neutral-950 font-semibold text-xs transition-all shadow-lg shadow-amber-500/20 active:scale-95 disabled:opacity-50 cursor-pointer"
          >
            <Download className={`w-4 h-4 ${isExporting ? 'animate-bounce' : ''}`} />
            {exportSuccess ? 'ZIP Exported!' : isExporting ? 'Packaging ZIP...' : 'Export to Antigravity (ZIP)'}
          </button>
        </div>
      </header>

      {/* Navigation Tabs Bar */}
      <div className="bg-neutral-950/80 border-b border-neutral-800 px-6 py-2 flex items-center gap-1 overflow-x-auto scrollbar-none">
        <button
          onClick={() => setActiveTab('export')}
          className={`flex items-center gap-1.5 px-3 py-1.5 rounded-md text-xs font-medium transition-all shrink-0 ${
            activeTab === 'export' 
              ? 'bg-amber-400 text-neutral-950 shadow font-semibold' 
              : 'text-amber-400/90 hover:text-amber-300 hover:bg-neutral-800 border border-amber-400/20'
          }`}
        >
          <FolderArchive className="w-3.5 h-3.5" />
          Antigravity Package
        </button>
        <button
          onClick={() => setActiveTab('preview')}
          className={`flex items-center gap-1.5 px-3 py-1.5 rounded-md text-xs font-medium transition-all shrink-0 ${
            activeTab === 'preview' 
              ? 'bg-amber-400 text-neutral-950 shadow font-semibold' 
              : 'text-neutral-400 hover:text-white hover:bg-neutral-800'
          }`}
        >
          <Eye className="w-3.5 h-3.5" />
          Simulator
        </button>
        <button
          onClick={() => setActiveTab('main')}
          className={`flex items-center gap-1.5 px-2.5 py-1.5 rounded-md text-xs font-medium transition-all shrink-0 ${
            activeTab === 'main' 
              ? 'bg-amber-400 text-neutral-950 shadow font-semibold' 
              : 'text-neutral-400 hover:text-white hover:bg-neutral-800'
          }`}
        >
          <Code2 className="w-3.5 h-3.5" />
          main.cpp
        </button>
        <button
          onClick={() => setActiveTab('pinconfig')}
          className={`flex items-center gap-1.5 px-2.5 py-1.5 rounded-md text-xs font-medium transition-all shrink-0 ${
            activeTab === 'pinconfig' 
              ? 'bg-amber-400 text-neutral-950 shadow font-semibold' 
              : 'text-neutral-400 hover:text-white hover:bg-neutral-800'
          }`}
        >
          <Settings className="w-3.5 h-3.5" />
          PinConfig.h
        </button>
        <button
          onClick={() => setActiveTab('sensor')}
          className={`flex items-center gap-1.5 px-2.5 py-1.5 rounded-md text-xs font-medium transition-all shrink-0 ${
            activeTab === 'sensor' 
              ? 'bg-amber-400 text-neutral-950 shadow font-semibold' 
              : 'text-neutral-400 hover:text-white hover:bg-neutral-800'
          }`}
        >
          <Sliders className="w-3.5 h-3.5" />
          SensorManager.h
        </button>
        <button
          onClick={() => setActiveTab('sensorcpp')}
          className={`flex items-center gap-1.5 px-2.5 py-1.5 rounded-md text-xs font-medium transition-all shrink-0 ${
            activeTab === 'sensorcpp' 
              ? 'bg-amber-400 text-neutral-950 shadow font-semibold' 
              : 'text-neutral-400 hover:text-white hover:bg-neutral-800'
          }`}
        >
          <Sliders className="w-3.5 h-3.5" />
          SensorManager.cpp
        </button>
        <button
          onClick={() => setActiveTab('motor')}
          className={`flex items-center gap-1.5 px-2.5 py-1.5 rounded-md text-xs font-medium transition-all shrink-0 ${
            activeTab === 'motor' 
              ? 'bg-amber-400 text-neutral-950 shadow font-semibold' 
              : 'text-neutral-400 hover:text-white hover:bg-neutral-800'
          }`}
        >
          <Cpu className="w-3.5 h-3.5" />
          MotorManager.h
        </button>
        <button
          onClick={() => setActiveTab('motorcpp')}
          className={`flex items-center gap-1.5 px-2.5 py-1.5 rounded-md text-xs font-medium transition-all shrink-0 ${
            activeTab === 'motorcpp' 
              ? 'bg-amber-400 text-neutral-950 shadow font-semibold' 
              : 'text-neutral-400 hover:text-white hover:bg-neutral-800'
          }`}
        >
          <Cpu className="w-3.5 h-3.5" />
          MotorManager.cpp
        </button>
        <button
          onClick={() => setActiveTab('scale')}
          className={`flex items-center gap-1.5 px-2.5 py-1.5 rounded-md text-xs font-medium transition-all shrink-0 ${
            activeTab === 'scale' 
              ? 'bg-amber-400 text-neutral-950 shadow font-semibold' 
              : 'text-neutral-400 hover:text-white hover:bg-neutral-800'
          }`}
        >
          <Scale className="w-3.5 h-3.5" />
          ScaleManager.h
        </button>
        <button
          onClick={() => setActiveTab('scalecpp')}
          className={`flex items-center gap-1.5 px-2.5 py-1.5 rounded-md text-xs font-medium transition-all shrink-0 ${
            activeTab === 'scalecpp' 
              ? 'bg-amber-400 text-neutral-950 shadow font-semibold' 
              : 'text-neutral-400 hover:text-white hover:bg-neutral-800'
          }`}
        >
          <Scale className="w-3.5 h-3.5" />
          ScaleManager.cpp
        </button>
        <button
          onClick={() => setActiveTab('sound')}
          className={`flex items-center gap-1.5 px-2.5 py-1.5 rounded-md text-xs font-medium transition-all shrink-0 ${
            activeTab === 'sound' 
              ? 'bg-amber-400 text-neutral-950 shadow font-semibold' 
              : 'text-neutral-400 hover:text-white hover:bg-neutral-800'
          }`}
        >
          SoundManager.h
        </button>
        <button
          onClick={() => setActiveTab('soundcpp')}
          className={`flex items-center gap-1.5 px-2.5 py-1.5 rounded-md text-xs font-medium transition-all shrink-0 ${
            activeTab === 'soundcpp' 
              ? 'bg-amber-400 text-neutral-950 shadow font-semibold' 
              : 'text-neutral-400 hover:text-white hover:bg-neutral-800'
          }`}
        >
          SoundManager.cpp
        </button>
        <button
          onClick={() => setActiveTab('header')}
          className={`flex items-center gap-1.5 px-2.5 py-1.5 rounded-md text-xs font-medium transition-all shrink-0 ${
            activeTab === 'header' 
              ? 'bg-amber-400 text-neutral-950 shadow font-semibold' 
              : 'text-neutral-400 hover:text-white hover:bg-neutral-800'
          }`}
        >
          DisplayManager.h
        </button>
        <button
          onClick={() => setActiveTab('cpp')}
          className={`flex items-center gap-1.5 px-2.5 py-1.5 rounded-md text-xs font-medium transition-all shrink-0 ${
            activeTab === 'cpp' 
              ? 'bg-amber-400 text-neutral-950 shadow font-semibold' 
              : 'text-neutral-400 hover:text-white hover:bg-neutral-800'
          }`}
        >
          DisplayManager.cpp
        </button>
        <button
          onClick={() => setActiveTab('platformio')}
          className={`flex items-center gap-1.5 px-2.5 py-1.5 rounded-md text-xs font-medium transition-all shrink-0 ${
            activeTab === 'platformio' 
              ? 'bg-amber-400 text-neutral-950 shadow font-semibold' 
              : 'text-neutral-400 hover:text-white hover:bg-neutral-800'
          }`}
        >
          <FileCode className="w-3.5 h-3.5" />
          platformio.ini
        </button>
        <button
          onClick={() => setActiveTab('manual')}
          className={`flex items-center gap-1.5 px-2.5 py-1.5 rounded-md text-xs font-medium transition-all shrink-0 ${
            activeTab === 'manual' 
              ? 'bg-amber-400 text-neutral-950 shadow font-semibold' 
              : 'text-amber-300 hover:text-white hover:bg-neutral-800 border border-amber-500/20'
          }`}
        >
          <Package className="w-3.5 h-3.5" />
          USER_MANUAL.md
        </button>
        <button
          onClick={() => setActiveTab('readme')}
          className={`flex items-center gap-1.5 px-2.5 py-1.5 rounded-md text-xs font-medium transition-all shrink-0 ${
            activeTab === 'readme' 
              ? 'bg-amber-400 text-neutral-950 shadow font-semibold' 
              : 'text-neutral-400 hover:text-white hover:bg-neutral-800'
          }`}
        >
          <Package className="w-3.5 h-3.5" />
          README.md
        </button>
      </div>

      {/* Main Content Area */}
      <div className="flex-1 p-6 max-w-7xl mx-auto w-full">
        {activeTab === 'export' && (
          <div className="space-y-6">
            {/* Hero Export Card */}
            <div className="bg-gradient-to-br from-neutral-950 via-neutral-900 to-neutral-950 border border-amber-500/30 rounded-2xl p-6 sm:p-8 shadow-2xl relative overflow-hidden">
              <div className="absolute top-0 right-0 w-96 h-96 bg-amber-500/5 rounded-full blur-3xl pointer-events-none"></div>
              
              <div className="flex flex-col md:flex-row md:items-center justify-between gap-6 relative z-10">
                <div className="space-y-3 max-w-2xl">
                  <div className="inline-flex items-center gap-2 px-3 py-1 rounded-full bg-amber-500/10 border border-amber-500/30 text-amber-400 text-xs font-medium">
                    <FolderArchive className="w-3.5 h-3.5" />
                    Google Antigravity Export Suite
                  </div>
                  <h2 className="text-2xl font-bold text-white tracking-tight">
                    Bee Mine Candle Dipper Firmware Package
                  </h2>
                  <p className="text-sm text-neutral-300 leading-relaxed">
                    Complete, self-contained PlatformIO C++ project pre-configured for the <strong>M5Stack CoreS3 SE</strong> and the <strong>Kerzenkiste 36-Candle Stainless Dipping Basket</strong>. Includes 2-Bucket Setup, 3-Card Vertical Scroll UI, a dedicated I2S <strong>SoundManager</strong> melody library (27 tunes + Volume), and a print-ready <strong>USER_MANUAL.md</strong>.
                  </p>
                  <div className="flex flex-wrap items-center gap-3 pt-1 text-xs text-neutral-400">
                    <span className="flex items-center gap-1.5 text-emerald-400">
                      <CheckCircle2 className="w-3.5 h-3.5" /> 2-Bucket Setup + 3-Card Scroll UI
                    </span>
                    <span className="flex items-center gap-1.5 text-emerald-400">
                      <CheckCircle2 className="w-3.5 h-3.5" /> Print-Ready USER_MANUAL.md
                    </span>
                    <span className="flex items-center gap-1.5 text-emerald-400">
                      <CheckCircle2 className="w-3.5 h-3.5" /> I2S SoundManager (27 Tunes + Volume)
                    </span>
                    <span className="flex items-center gap-1.5 text-emerald-400">
                      <CheckCircle2 className="w-3.5 h-3.5" /> Two-Stage Speeds &amp; Drip-Off
                    </span>
                  </div>
                </div>

                <div className="flex flex-col sm:flex-row md:flex-col gap-3 shrink-0">
                  <button
                    onClick={handleExportZip}
                    disabled={isExporting}
                    className="flex items-center justify-center gap-2 px-6 py-3.5 rounded-xl bg-gradient-to-r from-amber-500 to-amber-400 hover:from-amber-400 hover:to-amber-300 text-neutral-950 font-bold text-sm transition-all shadow-xl shadow-amber-500/25 active:scale-95 disabled:opacity-50 cursor-pointer"
                  >
                    <Download className={`w-4 h-4 ${isExporting ? 'animate-bounce' : ''}`} />
                    {exportSuccess ? 'ZIP Downloaded!' : isExporting ? 'Creating ZIP...' : 'Download Project ZIP Bundle'}
                  </button>
                  <button
                    onClick={() => setActiveTab('preview')}
                    className="flex items-center justify-center gap-2 px-6 py-3 rounded-xl bg-neutral-800 hover:bg-neutral-700 text-neutral-200 font-semibold text-xs transition-all border border-neutral-700 cursor-pointer"
                  >
                    <Eye className="w-4 h-4 text-amber-400" />
                    Open CoreS3 Simulator View
                  </button>
                </div>
              </div>
            </div>

            {/* Architecture & Workflow Cards */}
            <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
              {/* Card 1: Hardware Pinout Verified */}
              <div className="bg-neutral-950 border border-neutral-800 rounded-xl p-5 flex flex-col justify-between">
                <div>
                  <div className="flex items-center justify-between mb-3">
                    <h3 className="text-sm font-semibold text-white flex items-center gap-2">
                      <ShieldCheck className="w-4 h-4 text-emerald-400" />
                      Hardware Integration Status
                    </h3>
                    <span className="text-[10px] px-2 py-0.5 rounded bg-emerald-500/10 text-emerald-400 border border-emerald-500/20 font-mono">
                      Verified
                    </span>
                  </div>
                  <ul className="text-xs space-y-2 text-neutral-300">
                    <li className="flex items-start justify-between border-b border-neutral-900 pb-1.5">
                      <span className="text-neutral-400 font-mono">GPIO 13 (Cap Sensor)</span>
                      <span className="text-emerald-400 font-medium">Wax Surface Detector</span>
                    </li>
                    <li className="flex items-start justify-between border-b border-neutral-900 pb-1.5">
                      <span className="text-neutral-400 font-mono">GPIO 7 (Optical Home)</span>
                      <span className="text-neutral-200 font-medium">Top 0.0mm Reference</span>
                    </li>
                    <li className="flex items-start justify-between border-b border-neutral-900 pb-1.5">
                      <span className="text-neutral-400 font-mono">GPIO 6, 5, 10</span>
                      <span className="text-neutral-200 font-medium">DM542T Step, Dir, Enable</span>
                    </li>
                    <li className="flex items-start justify-between border-b border-neutral-900 pb-1.5">
                      <span className="text-neutral-400 font-mono">GPIO 2, 1 (Port A)</span>
                      <span className="text-amber-400 font-medium">HX711 Load Cell (DOUT/SCK)</span>
                    </li>
                    <li className="flex items-start justify-between">
                      <span className="text-neutral-400 font-mono">AW88298 I2S Amp</span>
                      <span className="text-neutral-200 font-medium">CoreS3 SE 1W Speaker</span>
                    </li>
                  </ul>
                </div>
                <div className="mt-4 pt-3 border-t border-neutral-800/80 text-[11px] text-neutral-400">
                  ⚡ <strong>36-Candle Jig:</strong> 22×22×62cm stainless basket (~720g empty weight) auto-tared in RAM at the start of every batch.
                </div>
              </div>

              {/* Card 2: Antigravity Commands */}
              <div className="bg-neutral-950 border border-neutral-800 rounded-xl p-5 flex flex-col justify-between">
                <div>
                  <div className="flex items-center justify-between mb-3">
                    <h3 className="text-sm font-semibold text-white flex items-center gap-2">
                      <Terminal className="w-4 h-4 text-amber-400" />
                      VS Code + PlatformIO Flashing
                    </h3>
                    <span className="text-[10px] px-2 py-0.5 rounded bg-neutral-800 text-neutral-400 font-mono">
                      PlatformIO
                    </span>
                  </div>
                  <div className="space-y-2 text-xs">
                    <div className="bg-neutral-900/90 rounded-lg p-2.5 border border-neutral-800 font-mono">
                      <div className="text-neutral-400 text-[10px] mb-1"># 1. Compile firmware</div>
                      <div className="text-amber-300 flex items-center justify-between">
                        <span>pio run</span>
                        <button onClick={() => copyCode('pio run', 'cmd1')} className="hover:text-white cursor-pointer">
                          {copiedFile === 'cmd1' ? <Check className="w-3 h-3 text-emerald-400" /> : <Copy className="w-3 h-3" />}
                        </button>
                      </div>
                    </div>
                    <div className="bg-neutral-900/90 rounded-lg p-2.5 border border-neutral-800 font-mono">
                      <div className="text-neutral-400 text-[10px] mb-1"># 2. Flash to CoreS3 via USB-C</div>
                      <div className="text-amber-300 flex items-center justify-between">
                        <span>pio run -t upload</span>
                        <button onClick={() => copyCode('pio run -t upload', 'cmd2')} className="hover:text-white cursor-pointer">
                          {copiedFile === 'cmd2' ? <Check className="w-3 h-3 text-emerald-400" /> : <Copy className="w-3 h-3" />}
                        </button>
                      </div>
                    </div>
                    <div className="bg-neutral-900/90 rounded-lg p-2.5 border border-neutral-800 font-mono">
                      <div className="text-neutral-400 text-[10px] mb-1"># 3. Live serial telemetry</div>
                      <div className="text-amber-300 flex items-center justify-between">
                        <span>pio device monitor -b 115200</span>
                        <button onClick={() => copyCode('pio device monitor -b 115200', 'cmd3')} className="hover:text-white cursor-pointer">
                          {copiedFile === 'cmd3' ? <Check className="w-3 h-3 text-emerald-400" /> : <Copy className="w-3 h-3" />}
                        </button>
                      </div>
                    </div>
                  </div>
                </div>
                <div className="mt-4 pt-3 border-t border-neutral-800/80 text-[11px] text-neutral-400">
                  Target environment: <code className="text-amber-300 font-mono">[env:m5stack-cores3]</code> in <code className="text-neutral-300 font-mono">platformio.ini</code>.
                </div>
              </div>

              {/* Card 3: Modular C++ Architecture */}
              <div className="bg-neutral-950 border border-neutral-800 rounded-xl p-5 flex flex-col justify-between">
                <div>
                  <div className="flex items-center justify-between mb-3">
                    <h3 className="text-sm font-semibold text-white flex items-center gap-2">
                      <Layers className="w-4 h-4 text-amber-400" />
                      Object-Oriented Subsystems
                    </h3>
                    <span className="text-[10px] px-2 py-0.5 rounded bg-amber-500/10 text-amber-400 border border-amber-500/20 font-mono">
                      Decoupled
                    </span>
                  </div>
                  <ul className="text-xs space-y-2 text-neutral-300">
                    <li className="flex items-start gap-2">
                      <span className="w-1.5 h-1.5 rounded-full bg-amber-400 mt-1.5 shrink-0"></span>
                      <div>
                        <strong className="text-white font-mono">DisplayManager:</strong> 2-Bucket Setup (`RECIPE` / `MACHINE`), 3-Card vertical list with hints &amp; right scrollbar.
                      </div>
                    </li>
                    <li className="flex items-start gap-2">
                      <span className="w-1.5 h-1.5 rounded-full bg-amber-400 mt-1.5 shrink-0"></span>
                      <div>
                        <strong className="text-white font-mono">SoundManager:</strong> Non-blocking AW88298 I2S synthesizer (&lt;2.5KB Flash) with 27 Startup/Finish/Alert tunes &amp; Volume.
                      </div>
                    </li>
                    <li className="flex items-start gap-2">
                      <span className="w-1.5 h-1.5 rounded-full bg-amber-400 mt-1.5 shrink-0"></span>
                      <div>
                        <strong className="text-white font-mono">MotorManager:</strong> Two-stage immersion/extraction speeds, Drip-Off, Base Melt &amp; Clean Whole Jig.
                      </div>
                    </li>
                    <li className="flex items-start gap-2">
                      <span className="w-1.5 h-1.5 rounded-full bg-amber-400 mt-1.5 shrink-0"></span>
                      <div>
                        <strong className="text-white font-mono">ScaleManager:</strong> RAM-only 0g Tare for different jigs + NVS-saved load cell calibration slope.
                      </div>
                    </li>
                  </ul>
                </div>
                <div className="mt-4 pt-3 border-t border-neutral-800/80 text-[11px] text-neutral-400">
                  Coordinator: <code className="text-amber-300 font-mono">main.cpp</code> state machine routes UI events and non-blocking cycles.
                </div>
              </div>
            </div>

            {/* Complete Project File Manifest Table */}
            <div className="bg-neutral-950 border border-neutral-800 rounded-xl overflow-hidden">
              <div className="px-6 py-4 border-b border-neutral-800 flex flex-col sm:flex-row sm:items-center justify-between gap-3 bg-neutral-900/50">
                <div>
                  <h3 className="text-sm font-semibold text-white flex items-center gap-2">
                    <FileCode className="w-4 h-4 text-amber-400" />
                    Complete Package File Manifest (16 Files)
                  </h3>
                  <p className="text-xs text-neutral-400">
                    All files are bundled in the root and <code className="text-amber-300 font-mono">src/</code> directory inside the export ZIP.
                  </p>
                </div>
                <button
                  onClick={handleExportZip}
                  disabled={isExporting}
                  className="flex items-center gap-1.5 px-3.5 py-1.5 rounded-lg bg-amber-400 hover:bg-amber-300 text-neutral-950 font-semibold text-xs transition-all cursor-pointer shrink-0"
                >
                  <Download className="w-3.5 h-3.5" />
                  Download All (ZIP)
                </button>
              </div>

              <div className="divide-y divide-neutral-900 text-xs">
                {[
                  { name: 'USER_MANUAL.md', path: '/USER_MANUAL.md', type: 'Manual', desc: 'Print-ready Operations & Configuration Manual: Executive Summary, Index, Speed Glossary, Sound & Tunes, Kerzenkiste 36-Candle Jig specs', code: userManualRaw, tab: 'manual' },
                  { name: 'PROJECT_HANDOVER.md', path: '/PROJECT_HANDOVER.md', type: 'Doc', desc: 'Comprehensive handover: full specs, circuits, modules, blockers, and next steps', code: handoverMdRaw, tab: 'readme' },
                  { name: 'platformio.ini', path: '/platformio.ini', type: 'Config', desc: 'PlatformIO CoreS3 board, espressif32 platform, and M5Unified library dependencies', code: platformioIniRaw, tab: 'platformio' },
                  { name: 'README.md', path: '/README.md', type: 'Doc', desc: 'Complete hardware schematics, PC817 wiring, pinout table, and build guide', code: readmeRaw, tab: 'readme' },
                  { name: 'main.cpp', path: '/src/main.cpp', type: 'C++ Main', desc: 'Automated dipping state machine, SoundManager integration, collision top-rest retraction, Base Melt, and Clean Jig', code: mainCppRaw, tab: 'main' },
                  { name: 'PinConfig.h', path: '/src/PinConfig.h', type: 'C++ Header', desc: 'Hardware GPIO definitions for DM542T, Capacitive Sensor, HX711, and top limit switch', code: pinConfigRaw, tab: 'pinconfig' },
                  { name: 'SoundManager.h', path: '/src/SoundManager.h', type: 'C++ Header', desc: 'Dedicated I2S speaker melody synthesizer header (8 Startup, 10 Finish, 9 Alert tunes + Volume)', code: soundManagerHRaw, tab: 'sound' },
                  { name: 'SoundManager.cpp', path: '/src/SoundManager.cpp', type: 'C++ Source', desc: 'Compact Flash note arrays (Dota 2 Ready, SpongeBob, Adventure Time, Classical, Heavy Metal, Mario, Bird, Dog Bark)', code: soundManagerCppRaw, tab: 'soundcpp' },
                  { name: 'SensorManager.h', path: '/src/SensorManager.h', type: 'C++ Header', desc: 'Glitch-filtered sensor class definition and active-LOW polarity configuration', code: sensorManagerHRaw, tab: 'sensor' },
                  { name: 'SensorManager.cpp', path: '/src/SensorManager.cpp', type: 'C++ Source', desc: 'Glitch-filtered debouncing for top optical switch and capacitive wax sensor', code: sensorManagerCppRaw, tab: 'sensorcpp' },
                  { name: 'MotorManager.h', path: '/src/MotorManager.h', type: 'C++ Header', desc: 'Stepper driver class, top-rest retraction, Melt Bases, and Clean Whole Jig', code: motorManagerHRaw, tab: 'motor' },
                  { name: 'MotorManager.cpp', path: '/src/MotorManager.cpp', type: 'C++ Source', desc: 'Non-blocking pulse generator, dynamic speed transitions, and limit switch backoff', code: motorManagerCppRaw, tab: 'motorcpp' },
                  { name: 'ScaleManager.h', path: '/src/ScaleManager.h', type: 'C++ Header', desc: 'HX711 24-bit strain-gauge ADC class, RAM-only Tare, and NVS slope persistence', code: scaleManagerHRaw, tab: 'scale' },
                  { name: 'ScaleManager.cpp', path: '/src/ScaleManager.cpp', type: 'C++ Source', desc: '10Hz bit-bang sampling, blocking multi-sample RAM Tare, and 2-point Load Cell Calibration', code: scaleManagerCppRaw, tab: 'scalecpp' },
                  { name: 'DisplayManager.h', path: '/src/DisplayManager.h', type: 'C++ Header', desc: 'Double-buffered M5Canvas graphics pipeline class, 2-Bucket Setup, and 3-Card Scroll UI', code: displayManagerHRaw, tab: 'header' },
                  { name: 'DisplayManager.cpp', path: '/src/DisplayManager.cpp', type: 'C++ Source', desc: '4-tab UI, 2-Bucket Setup (Recipe vs Machine), 3-Card Vertical Scroll with hints, Sound & Tunes page, and 3 palettes', code: displayManagerCppRaw, tab: 'cpp' },
                ].map((file) => (
                  <div key={file.path} className="px-6 py-3 flex flex-col sm:flex-row sm:items-center justify-between gap-3 hover:bg-neutral-900/40 transition-colors">
                    <div className="flex items-start sm:items-center gap-3">
                      <div className="w-8 h-8 rounded-lg bg-neutral-900 border border-neutral-800 flex items-center justify-center shrink-0">
                        <FileCode className="w-4 h-4 text-amber-400" />
                      </div>
                      <div>
                        <div className="flex items-center gap-2">
                          <span className="font-mono font-semibold text-white">{file.path}</span>
                          <span className={`text-[10px] px-1.5 py-0.2 rounded font-mono ${
                            file.type.includes('Header') ? 'bg-blue-500/10 text-blue-400 border border-blue-500/20' :
                            file.type.includes('Source') || file.type.includes('Main') ? 'bg-amber-500/10 text-amber-400 border border-amber-500/20' :
                            'bg-purple-500/10 text-purple-400 border border-purple-500/20'
                          }`}>
                            {file.type}
                          </span>
                        </div>
                        <p className="text-[11px] text-neutral-400 mt-0.5">{file.desc}</p>
                      </div>
                    </div>

                    <div className="flex items-center gap-2 self-end sm:self-center shrink-0">
                      <button
                        onClick={() => setActiveTab(file.tab as any)}
                        className="px-2.5 py-1 rounded bg-neutral-900 hover:bg-neutral-800 text-[11px] text-neutral-300 hover:text-white border border-neutral-800 transition-colors cursor-pointer"
                      >
                        View Code
                      </button>
                      <button
                        onClick={() => downloadSingleFile(file.name, file.code || '')}
                        className="flex items-center gap-1 px-2.5 py-1 rounded bg-neutral-900 hover:bg-neutral-800 text-[11px] text-amber-400 border border-neutral-800 transition-colors cursor-pointer"
                        title="Download file"
                      >
                        <Download className="w-3 h-3" />
                        Download
                      </button>
                      <button
                        onClick={() => copyCode(file.code || '', file.name)}
                        className="flex items-center gap-1 px-2.5 py-1 rounded bg-neutral-900 hover:bg-neutral-800 text-[11px] text-neutral-300 hover:text-white border border-neutral-800 transition-colors cursor-pointer"
                        title="Copy code"
                      >
                        {copiedFile === file.name ? <Check className="w-3 h-3 text-emerald-400" /> : <Copy className="w-3 h-3" />}
                        {copiedFile === file.name ? 'Copied' : 'Copy'}
                      </button>
                    </div>
                  </div>
                ))}
              </div>
            </div>
          </div>
        )}

        {activeTab === 'preview' && (
          <div className="grid grid-cols-1 lg:grid-cols-12 gap-6">
            {/* CoreS3 Hardware Device Frame */}
            <div className="lg:col-span-7 flex flex-col items-center justify-center">
              <div className="relative bg-neutral-800 border-4 border-neutral-700 rounded-3xl p-6 shadow-2xl max-w-md w-full">
                {/* M5Stack CoreS3 Logo & Status */}
                <div className="flex items-center justify-between mb-4 px-2">
                  <div className="flex items-center gap-2">
                    <span className="text-xs font-bold font-mono tracking-widest text-neutral-300">
                      M5STACK CORES3 SE
                    </span>
                  </div>
                  <div className="flex items-center gap-2">
                    <span className="w-2 h-2 rounded-full bg-emerald-400 animate-pulse"></span>
                    <span className="text-[10px] text-neutral-400 font-mono">320x240 IPS Touch</span>
                  </div>
                </div>

                {/* Simulated 320x240 Display Screen */}
                <div 
                  className="w-full aspect-[4/3] rounded-xl overflow-hidden relative shadow-inner border border-neutral-950 select-none cursor-pointer"
                  style={{ backgroundColor: ts.bg }}
                >
                  {/* Top Banner (15% height = 36px) */}
                  <div 
                    className="h-[36px] w-full px-3 flex items-center justify-between text-xs font-bold font-mono"
                    style={{ backgroundColor: ts.bannerBg, color: ts.bannerTxt }}
                  >
                    {page === 2 ? (
                      <>
                        <span>POS: {posMM.toFixed(1)}mm</span>
                        <span>WT: {weight.toFixed(0)}g</span>
                      </>
                    ) : page === 14 ? (
                      <>
                        <span>WAX: {lastWaxLevelMM > 0 ? `${lastWaxLevelMM.toFixed(0)}mm` : '---'}</span>
                        <span>BASE: {lastWaxLevelMM > settingValues[14] ? `${(lastWaxLevelMM - settingValues[14]).toFixed(0)}mm` : 'DIP 1ST'}</span>
                      </>
                    ) : page === 13 ? (
                      <>
                        <span className="font-sans">SCALE CALIBRATION</span>
                        <span>WT: {weight.toFixed(1)}g</span>
                      </>
                    ) : page === 3 ? (
                      <>
                        <span className="font-sans">SETUP — SELECT GROUP</span>
                        <span className="font-sans text-[10px] opacity-80">2 BUCKETS</span>
                      </>
                    ) : page === 30 ? (
                      <>
                        <span className="font-sans">1. RECIPE PARAMS</span>
                        <span className="font-sans text-[10px] opacity-80">PAGE {Math.floor(scrollOffset / 3) + 1}/2</span>
                      </>
                    ) : page === 31 ? (
                      <>
                        <span className="font-sans">2. MACHINE SETUP</span>
                        <span className="font-sans text-[10px] opacity-80">PAGE {Math.floor(scrollOffset / 3) + 1}/2</span>
                      </>
                    ) : (page === 4 || page === 5 || page === 50 || page === 51 || page === 52) ? (
                      <>
                        <span className="font-sans">
                          {page === 4 ? 'TARGET WEIGHTS & DIPS' : page === 5 ? 'DIP & DRIP TIMING' : page === 50 ? 'COOLING & FINISHING' : page === 51 ? 'TRAVEL & WAX SPEEDS' : 'JIG & WAX GEOMETRY'}
                        </span>
                      </>
                    ) : page === 9 ? (
                      <span className="mx-auto font-sans">DIPPING IN PROGRESS</span>
                    ) : page === 10 ? (
                      <span className="mx-auto font-sans">
                        {collisionAlert ? 'WAX COLLISION ALERT!' : lowWaxAlert ? 'LOW WAX ALERT' : 'CONFIRM ABORT'}
                      </span>
                    ) : page === 11 ? (
                      <span className="mx-auto font-sans">{dipAborted ? 'PROCESS CANCELLED' : 'PROCESS COMPLETE'}</span>
                    ) : (
                      <>
                        <span className="font-sans">
                          {lastWaxLevelMM >= settingValues[9] - 20 ? `! LOW WAX (${lastWaxLevelMM.toFixed(0)}mm)` : 'Thu 01/01/26'}
                        </span>
                        <span className="font-sans">15:10</span>
                      </>
                    )}
                  </div>

                  {/* Main Page Content (Center 70% height = 168px) */}
                  <div className="h-[168px] w-full p-2 flex flex-col justify-center items-center relative">
                    {/* Page 0: Unified 1-Screen Dipping Launcher + [x] MELT BASES */}
                    {page === 0 && (
                      <div className="w-full h-full flex flex-col justify-between p-1 gap-1.5">
                        {/* Mode Toggle Pills */}
                        <div className="grid grid-cols-2 gap-2">
                          <button
                            onClick={() => { setActiveIsWeight(true); setLastEvent('Switched to Weight Mode'); }}
                            className="py-1 rounded-lg font-bold text-[11px] transition-all"
                            style={{
                              backgroundColor: activeIsWeight ? ts.btn1Bg : ts.outlineBg,
                              color: activeIsWeight ? ts.btnTxt : '#ffffff'
                            }}
                          >
                            MODE: WEIGHT
                          </button>
                          <button
                            onClick={() => { setActiveIsWeight(false); setLastEvent('Switched to Dips Mode'); }}
                            className="py-1 rounded-lg font-bold text-[11px] transition-all"
                            style={{
                              backgroundColor: !activeIsWeight ? ts.btn1Bg : ts.outlineBg,
                              color: !activeIsWeight ? ts.btnTxt : '#ffffff'
                            }}
                          >
                            MODE: DIPS
                          </button>
                        </div>

                        {/* Side-by-Side Profile Cards */}
                        <div className="grid grid-cols-2 gap-2 flex-1">
                          <button
                            onClick={() => { setActiveIsSlim(true); setLastEvent('Selected SLIM profile'); }}
                            className={`rounded-xl font-bold text-xs flex flex-col items-center justify-center gap-0.5 transition-all ${activeIsSlim ? 'ring-2 ring-white' : ''}`}
                            style={{
                              backgroundColor: activeIsSlim ? ts.btn1Bg : ts.outlineBg,
                              color: activeIsSlim ? ts.btnTxt : '#ffffff'
                            }}
                          >
                            <span>SLIM</span>
                            <span className="text-[11px] opacity-90">
                              {activeIsWeight ? `${settingValues[0]} g` : `${settingValues[2]} dips`}
                            </span>
                            <span className="text-[9px] opacity-75">Est: 02:15</span>
                          </button>
                          <button
                            onClick={() => { setActiveIsSlim(false); setLastEvent('Selected STANDARD profile'); }}
                            className={`rounded-xl font-bold text-xs flex flex-col items-center justify-center gap-0.5 transition-all ${!activeIsSlim ? 'ring-2 ring-white' : ''}`}
                            style={{
                              backgroundColor: !activeIsSlim ? ts.btn2Bg : ts.outlineBg,
                              color: !activeIsSlim ? ts.btnTxt : '#ffffff'
                            }}
                          >
                            <span>STANDARD</span>
                            <span className="text-[11px] opacity-90">
                              {activeIsWeight ? `${settingValues[1]} g` : `${settingValues[3]} dips`}
                            </span>
                            <span className="text-[9px] opacity-75">Est: 03:40</span>
                          </button>
                        </div>

                        {/* Bottom Row: [x] MELT BASES Checkbox + START DIP Button */}
                        <div className="grid grid-cols-12 gap-2">
                          <button
                            onClick={() => {
                              setMeltBasesEnabled(!meltBasesEnabled);
                              setLastEvent(`Final-Stage Base Melt: ${!meltBasesEnabled ? 'ENABLED' : 'OFF'}`);
                            }}
                            className="col-span-6 py-2 rounded-xl font-bold text-[10px] shadow-md transition-all active:scale-95 flex items-center justify-center"
                            style={{
                              backgroundColor: meltBasesEnabled ? ts.btn2Bg : ts.outlineBg,
                              color: meltBasesEnabled ? ts.btnTxt : '#ffffff'
                            }}
                          >
                            {meltBasesEnabled ? '[x] MELT BASES' : '[ ] MELT BASES'}
                          </button>
                          <button
                            onClick={() => {
                              setWeight(0);
                              setPage(9);
                              setDipAborted(false);
                              setCollisionAlert(false);
                              setLowWaxAlert(false);
                              setLastEvent(`Auto-Tared Jig in RAM (0g) & Started ${activeIsSlim ? 'Slim' : 'Standard'} Dip`);
                            }}
                            className="col-span-6 py-2 rounded-xl font-bold text-xs shadow-md transition-all active:scale-95 flex items-center justify-center"
                            style={{ backgroundColor: ts.activeBg, color: '#000000' }}
                          >
                            START DIP
                          </button>
                        </div>
                      </div>
                    )}

                    {/* Page 2: Manual Control */}
                    {page === 2 && (
                      <div className="w-full h-full grid grid-cols-12 gap-2 p-1">
                        <div className="col-span-7 flex flex-col justify-between">
                          <button
                            onMouseDown={() => startManualMove(true)}
                            onMouseUp={stopManualMove}
                            onMouseLeave={stopManualMove}
                            onTouchStart={() => startManualMove(true)}
                            onTouchEnd={stopManualMove}
                            className={`h-11 rounded-xl flex items-center justify-center font-bold text-xs select-none ${limitSwitch ? 'ring-2 ring-emerald-400' : ''}`}
                            style={{ backgroundColor: ts.btn1Bg, color: ts.btnTxt }}
                          >
                            ▲ UP
                          </button>
                          <div className="flex items-center justify-between px-2 text-xs font-bold" style={{ color: ts.btnTxt }}>
                            <button onClick={() => setSpeed(Math.max(5, speed - 5))} className="w-8 h-8 rounded bg-neutral-700 text-white font-bold flex items-center justify-center">-</button>
                            <span>{speed}</span>
                            <button onClick={() => setSpeed(Math.min(100, speed + 5))} className="w-8 h-8 rounded bg-neutral-700 text-white font-bold flex items-center justify-center">+</button>
                          </div>
                          <button
                            onMouseDown={() => startManualMove(false)}
                            onMouseUp={stopManualMove}
                            onMouseLeave={stopManualMove}
                            onTouchStart={() => startManualMove(false)}
                            onTouchEnd={stopManualMove}
                            className="h-11 rounded-xl flex items-center justify-center font-bold text-xs select-none"
                            style={{ backgroundColor: ts.btn1Bg, color: ts.btnTxt }}
                          >
                            ▼ DOWN
                          </button>
                        </div>
                        <div className="col-span-5 flex flex-col justify-between gap-1">
                          <button
                            onClick={() => {
                              if (isHoming) {
                                stopHoming();
                              } else {
                                setIsHoming(true);
                                setLastEvent('Homing Stage 1: Rapid search towards top limit switch...');
                                const t1 = setTimeout(() => {
                                  setLastEvent('Homing Stage 2: Backing down 2mm & slow precision re-approach...');
                                  const t2 = setTimeout(() => {
                                    setPosMM(0.0);
                                    setLimitSwitch(true);
                                    setIsHoming(false);
                                    setLastEvent('Homing complete: Position zeroed to 0.0mm');
                                  }, 800);
                                  homingTimerRef.current.push(t2);
                                }, 800);
                                homingTimerRef.current = [t1];
                              }
                            }}
                            className="flex-1 rounded-lg text-xs font-bold flex items-center justify-center transition-all"
                            style={{ 
                              backgroundColor: isHoming ? '#ef4444' : (limitSwitch ? ts.activeBg : ts.outlineBg), 
                              color: isHoming ? '#ffffff' : (limitSwitch ? '#000000' : '#ffffff') 
                            }}
                          >
                            {isHoming ? 'STOP' : 'HOME'}
                          </button>
                          <button
                            onClick={() => {
                              if (isDipBot) {
                                stopDipBot();
                              } else {
                                setIsDipBot(true);
                                setLastEvent('Dip Bot Stage 1: Lowering to capacitive wax sensor...');
                                const t1 = setTimeout(() => {
                                  setPosMM(360.0);
                                  setLastWaxLevelMM(360.0);
                                  setCapSensor(true);
                                  setLastEvent('Dip Bot Stage 2: Wax sensed at 360mm, holding...');
                                  const t2 = setTimeout(() => {
                                    setCapSensor(false);
                                    setLastEvent('Dip Bot Stage 3: Returning to 0.0mm top rest...');
                                    const t3 = setTimeout(() => {
                                      setPosMM(0.0);
                                      setLimitSwitch(true);
                                      setIsDipBot(false);
                                      setLastEvent('Dip Bot complete: Wax level saved (360mm)');
                                    }, 800);
                                    dipBotTimerRef.current.push(t3);
                                  }, 800);
                                  dipBotTimerRef.current.push(t2);
                                }, 800);
                                dipBotTimerRef.current = [t1];
                              }
                            }}
                            className="flex-1 rounded-lg text-xs font-bold flex items-center justify-center transition-all"
                            style={{ 
                              backgroundColor: isDipBot ? '#ef4444' : (capSensor ? ts.activeBg : ts.outlineBg), 
                              color: isDipBot ? '#ffffff' : (capSensor ? '#000000' : '#ffffff') 
                            }}
                          >
                            {isDipBot ? 'STOP' : 'DIP BOT'}
                          </button>
                          <button
                            onClick={() => { setWeight(0); setLastEvent('RAM Tare executed: Current jig zeroed (0.0g)'); }}
                            className="flex-1 rounded-lg text-xs font-bold bg-amber-500 text-black flex items-center justify-center"
                          >
                            TARE
                          </button>
                        </div>
                      </div>
                    )}

                    {/* Page 14: Jig & Base Tools (Melt Candle Bases + Clean Whole Jig) */}
                    {page === 14 && (
                      <div className="w-full h-full flex flex-col justify-between p-1 gap-1.5">
                        <button
                          onClick={() => {
                            if (isMeltingBases) {
                              stopDipBot();
                            } else {
                              const baseDepth = Math.max(0, lastWaxLevelMM - settingValues[14]);
                              setIsMeltingBases(true);
                              setPosMM(baseDepth);
                              setLastEvent(`Melting Candle Bases at Z=${baseDepth}mm (${lastWaxLevelMM}mm - ${settingValues[14]}mm) for ${settingValues[15]}s...`);
                              const t1 = setTimeout(() => {
                                setPosMM(0.0);
                                setIsMeltingBases(false);
                                setLastEvent('Melt Candle Bases complete: Returned to 0.0mm top rest');
                              }, 1200);
                              dipBotTimerRef.current = [t1];
                            }
                          }}
                          className="w-full py-2.5 rounded-xl font-bold text-xs flex items-center justify-center transition-all"
                          style={{
                            backgroundColor: isMeltingBases ? '#dc2626' : ts.btn1Bg,
                            color: isMeltingBases ? '#ffffff' : ts.btnTxt
                          }}
                        >
                          {isMeltingBases ? 'STOP MELTING BASES' : `MELT CANDLE BASES (${settingValues[15]}s)`}
                        </button>

                        <button
                          onClick={() => {
                            if (isCleaningJig) {
                              stopDipBot();
                            } else {
                              setIsCleaningJig(true);
                              setPosMM(360.0);
                              setLastWaxLevelMM(360.0);
                              setLastEvent(`Cleaning Whole Jig at full sensor depth (Z=360mm) for ${settingValues[16]}s...`);
                              const t1 = setTimeout(() => {
                                setPosMM(0.0);
                                setIsCleaningJig(false);
                                setLastEvent('Clean Whole Jig complete: Returned to 0.0mm top rest');
                              }, 1200);
                              dipBotTimerRef.current = [t1];
                            }
                          }}
                          className="w-full py-2.5 rounded-xl font-bold text-xs flex items-center justify-center transition-all"
                          style={{
                            backgroundColor: isCleaningJig ? '#dc2626' : ts.btn2Bg,
                            color: isCleaningJig ? '#ffffff' : ts.btnTxt
                          }}
                        >
                          {isCleaningJig ? 'STOP CLEANING JIG' : `CLEAN WHOLE JIG (${settingValues[16]}s)`}
                        </button>

                        <div className="grid grid-cols-2 gap-2">
                          <button
                            onClick={() => {
                              setMeltBasesEnabled(!meltBasesEnabled);
                              setLastEvent(`Auto Base Melt at end of dip: ${!meltBasesEnabled ? 'ON' : 'OFF'}`);
                            }}
                            className="py-1.5 rounded-lg font-bold text-[10px]"
                            style={{
                              backgroundColor: meltBasesEnabled ? ts.activeBg : ts.outlineBg,
                              color: meltBasesEnabled ? '#000000' : '#ffffff'
                            }}
                          >
                            {meltBasesEnabled ? 'AUTO END: ON' : 'AUTO END: OFF'}
                          </button>
                          <button
                            onClick={() => setPage(52)}
                            className="py-1.5 rounded-lg font-bold text-[10px] text-white"
                            style={{ backgroundColor: ts.outlineBg }}
                          >
                            JIG LEN &amp; TIMES &gt;
                          </button>
                        </div>
                      </div>
                    )}

                    {/* Page 3: 2-Bucket Top-Level Setup Hub (1. RECIPE PARAMS vs 2. MACHINE SETUP) */}
                    {page === 3 && (
                      <div className="w-full h-full flex flex-col justify-between p-1 gap-2">
                        <button
                          onClick={() => { setScrollOffset(0); setPage(30); setLastEvent('Opened 1. RECIPE PARAMS (3-Card Scroll)'); }}
                          className="flex-1 rounded-xl px-3 flex flex-col justify-center text-left transition-all shadow"
                          style={{ backgroundColor: ts.btn1Bg, color: ts.btnTxt }}
                        >
                          <div className="font-bold text-xs flex items-center justify-between w-full">
                            <span>1. RECIPE PARAMS</span>
                            <span>&gt;</span>
                          </div>
                          <span className="text-[10px] opacity-80 mt-0.5">
                            Weights, Dips, Speeds, Drip &amp; Cooling Curve
                          </span>
                        </button>
                        <button
                          onClick={() => { setScrollOffset(0); setPage(31); setLastEvent('Opened 2. MACHINE SETUP (3-Card Scroll)'); }}
                          className="flex-1 rounded-xl px-3 flex flex-col justify-center text-left transition-all shadow"
                          style={{ backgroundColor: ts.btn2Bg, color: ts.btnTxt }}
                        >
                          <div className="font-bold text-xs flex items-center justify-between w-full">
                            <span>2. MACHINE SETUP</span>
                            <span>&gt;</span>
                          </div>
                          <span className="text-[10px] opacity-80 mt-0.5">
                            Jig &amp; Limit, Load Cell Cal, Motor, Theme &amp; Wi-Fi
                          </span>
                        </button>
                      </div>
                    )}

                    {/* Page 30: Bucket 1 — RECIPE PARAMS (5 Categories in 3-Card Vertical Scroll + Right Scrollbar) */}
                    {page === 30 && (
                      <div className="w-full h-full flex gap-1.5 p-0.5">
                        <div className="flex-1 flex flex-col justify-between gap-1">
                          {[
                            { title: '1. Target Weights & Dips >', hint: 'Slim/Std net grams & dip counts', target: 4, primary: true },
                            { title: '2. Travel & Wax Speeds >', hint: 'Up, Down, Immerse (-15mm) & Extract', target: 51, primary: true },
                            { title: '3. Dip & Drip Timing >', hint: '1st/Sub dip dwell & Drip-Off offset', target: 5, primary: false },
                            { title: '4. Cooling & Finishing >', hint: 'Tapered cooling curve & Base Melt', target: 50, primary: false },
                          ].slice(scrollOffset, scrollOffset + 3).map((cat, i) => (
                            <button
                              key={i}
                              onClick={() => { setScrollOffset(0); setPage(cat.target); setLastEvent(`Opened ${cat.title}`); }}
                              className="h-[48px] w-full rounded-lg px-2.5 flex flex-col justify-center text-left"
                              style={{ backgroundColor: cat.primary ? ts.btn1Bg : ts.btn2Bg, color: ts.btnTxt }}
                            >
                              <span className="font-bold text-[11px] leading-tight">{cat.title}</span>
                              <span className="text-[9px] opacity-80 leading-tight mt-0.5">{cat.hint}</span>
                            </button>
                          ))}
                        </div>
                        <div className="w-[42px] flex flex-col justify-between gap-1">
                          <button
                            onClick={() => setScrollOffset(Math.max(0, scrollOffset - 3))}
                            className="h-[48px] rounded-lg font-bold text-xs flex items-center justify-center"
                            style={{ backgroundColor: scrollOffset > 0 ? ts.btn1Bg : ts.outlineBg, color: scrollOffset > 0 ? ts.btnTxt : '#ffffff' }}
                          >
                            ▲
                          </button>
                          <div className="h-[48px] rounded-lg flex items-center justify-center text-[10px] font-mono text-white" style={{ backgroundColor: ts.outlineBg }}>
                            {Math.floor(scrollOffset / 3) + 1}/2
                          </div>
                          <button
                            onClick={() => setScrollOffset(scrollOffset + 3 < 4 ? scrollOffset + 3 : scrollOffset)}
                            className="h-[48px] rounded-lg font-bold text-xs flex items-center justify-center"
                            style={{ backgroundColor: scrollOffset + 3 < 4 ? ts.btn1Bg : ts.outlineBg, color: scrollOffset + 3 < 4 ? ts.btnTxt : '#ffffff' }}
                          >
                            ▼
                          </button>
                        </div>
                      </div>
                    )}

                    {/* Page 31: Bucket 2 — MACHINE SETUP (6 Categories in 3-Card Vertical Scroll + Right Scrollbar) */}
                    {page === 31 && (
                      <div className="w-full h-full flex gap-1.5 p-0.5">
                        <div className="flex-1 flex flex-col justify-between gap-1">
                          {[
                            { title: '1. Jig & Wax Geometry >', hint: 'Soft Limit, Jig Length, Cutoff & Clean', target: 52, primary: true },
                            { title: '2. Scale & Collision >', hint: 'Collision limit & 2-Step Load Cell Cal', target: 6, primary: true },
                            { title: '3. Stepper Motor >', hint: 'Manual speed, Enable/Disable & Home', target: 12, primary: false },
                            { title: '4. Screen & Theme >', hint: 'Backlight brightness & 3 color palettes', target: 7, primary: false },
                            { title: '5. Wi-Fi Portal >', hint: 'Launch SoftAP captive setup portal', target: 8, primary: true },
                            { title: '6. Sound & Tunes >', hint: 'Volume & Startup / Finish / Alert melodies', target: 53, primary: false },
                          ].slice(scrollOffset, scrollOffset + 3).map((cat, i) => (
                            <button
                              key={i}
                              onClick={() => { setScrollOffset(0); setPage(cat.target); setLastEvent(`Opened ${cat.title}`); }}
                              className="h-[48px] w-full rounded-lg px-2.5 flex flex-col justify-center text-left"
                              style={{ backgroundColor: cat.primary ? ts.btn1Bg : ts.btn2Bg, color: ts.btnTxt }}
                            >
                              <span className="font-bold text-[11px] leading-tight">{cat.title}</span>
                              <span className="text-[9px] opacity-80 leading-tight mt-0.5">{cat.hint}</span>
                            </button>
                          ))}
                        </div>
                        <div className="w-[42px] flex flex-col justify-between gap-1">
                          <button
                            onClick={() => setScrollOffset(Math.max(0, scrollOffset - 3))}
                            className="h-[48px] rounded-lg font-bold text-xs flex items-center justify-center"
                            style={{ backgroundColor: scrollOffset > 0 ? ts.btn1Bg : ts.outlineBg, color: scrollOffset > 0 ? ts.btnTxt : '#ffffff' }}
                          >
                            ▲
                          </button>
                          <div className="h-[48px] rounded-lg flex items-center justify-center text-[10px] font-mono text-white" style={{ backgroundColor: ts.outlineBg }}>
                            {Math.floor(scrollOffset / 3) + 1}/2
                          </div>
                          <button
                            onClick={() => setScrollOffset(scrollOffset + 3 < 6 ? scrollOffset + 3 : scrollOffset)}
                            className="h-[48px] rounded-lg font-bold text-xs flex items-center justify-center"
                            style={{ backgroundColor: scrollOffset + 3 < 6 ? ts.btn1Bg : ts.outlineBg, color: scrollOffset + 3 < 6 ? ts.btnTxt : '#ffffff' }}
                          >
                            ▼
                          </button>
                        </div>
                      </div>
                    )}

                    {/* Page 53: Sound & Tunes (3-Card Vertical Scroll: Volume, Startup Tune, Finish Tune, Alert Tune) */}
                    {page === 53 && (
                      <div className="w-full h-full flex gap-1.5 p-0.5">
                        <div className="flex-1 flex flex-col justify-between gap-1">
                          {[0, 1, 2, 3].slice(scrollOffset, scrollOffset + 3).map((itemIdx) => {
                            const startupNames = ['Off (Silent)', 'Standard', 'Dota 2 Ready', 'SpongeBob', 'Adv. Time', 'Classical', 'Heavy Metal', 'Mario 1-Up'];
                            const finishNames = ['Off (Silent)', 'Standard', 'Dota 2 Ready', 'SpongeBob', 'Adv. Time', 'Classical', 'Heavy Metal', 'Kitchen Timer', 'iPhone Ding', 'Bird Song'];
                            const alertNames = ['Off (Silent)', 'Standard', 'Mario Death', 'Dog Bark', 'Bird Chirp', 'Timer Ring', 'iPhone Ding', 'Metal Doom', 'Retro Fail'];

                            if (itemIdx === 0) {
                              return (
                                <button
                                  key="vol"
                                  onClick={() => {
                                    const nextV = volume >= 100 ? 0 : volume + 20;
                                    setVolume(nextV);
                                    setLastEvent(`Speaker Volume: ${nextV === 0 ? 'MUTE (0%)' : `${nextV}%`}`);
                                  }}
                                  className="h-[48px] w-full rounded-lg px-2.5 flex flex-col justify-center text-left"
                                  style={{ backgroundColor: ts.btn1Bg, color: ts.btnTxt }}
                                >
                                  <div className="w-full flex items-center justify-between font-bold text-[11px]">
                                    <span>Speaker Volume:</span>
                                    <span>{volume === 0 ? 'MUTE (0%)' : `${volume}%`}</span>
                                  </div>
                                  <div className="w-full h-2.5 rounded-full mt-1 overflow-hidden" style={{ backgroundColor: ts.outlineBg }}>
                                    <div className="h-full rounded-full" style={{ width: `${volume}%`, backgroundColor: ts.activeBg }} />
                                  </div>
                                </button>
                              );
                            } else if (itemIdx === 1) {
                              return (
                                <button
                                  key="st"
                                  onClick={() => {
                                    const next = (startupTune + 1) % startupNames.length;
                                    setStartupTune(next);
                                    setLastEvent(`Startup Tune: ${startupNames[next]} (Live Preview Playing!)`);
                                  }}
                                  className="h-[48px] w-full rounded-lg px-2.5 flex flex-col justify-center text-left"
                                  style={{ backgroundColor: ts.btn2Bg, color: ts.btnTxt }}
                                >
                                  <div className="w-full flex items-center justify-between font-bold text-[11px]">
                                    <span>Startup Tune:</span>
                                    <span>{startupNames[startupTune]}</span>
                                  </div>
                                  <span className="text-[9px] opacity-80 mt-0.5">Tap to cycle &amp; test boot melody</span>
                                </button>
                              );
                            } else if (itemIdx === 2) {
                              return (
                                <button
                                  key="fn"
                                  onClick={() => {
                                    const next = (finishTune + 1) % finishNames.length;
                                    setFinishTune(next);
                                    setLastEvent(`Finish Tune: ${finishNames[next]} (Live Preview Playing!)`);
                                  }}
                                  className="h-[48px] w-full rounded-lg px-2.5 flex flex-col justify-center text-left"
                                  style={{ backgroundColor: ts.btn1Bg, color: ts.btnTxt }}
                                >
                                  <div className="w-full flex items-center justify-between font-bold text-[11px]">
                                    <span>Finish Tune:</span>
                                    <span>{finishNames[finishTune]}</span>
                                  </div>
                                  <span className="text-[9px] opacity-80 mt-0.5">Tap to cycle &amp; test batch complete melody</span>
                                </button>
                              );
                            } else {
                              return (
                                <button
                                  key="al"
                                  onClick={() => {
                                    const next = (alertTune + 1) % alertNames.length;
                                    setAlertTune(next);
                                    setLastEvent(`Alert Tune: ${alertNames[next]} (Live Preview Playing!)`);
                                  }}
                                  className="h-[48px] w-full rounded-lg px-2.5 flex flex-col justify-center text-left"
                                  style={{ backgroundColor: ts.btn2Bg, color: ts.btnTxt }}
                                >
                                  <div className="w-full flex items-center justify-between font-bold text-[11px]">
                                    <span>Alert Tune:</span>
                                    <span>{alertNames[alertTune]}</span>
                                  </div>
                                  <span className="text-[9px] opacity-80 mt-0.5">Tap to cycle &amp; test collision/error alarm</span>
                                </button>
                              );
                            }
                          })}
                        </div>
                        <div className="w-[42px] flex flex-col justify-between gap-1">
                          <button
                            onClick={() => setScrollOffset(Math.max(0, scrollOffset - 3))}
                            className="h-[48px] rounded-lg font-bold text-xs flex items-center justify-center"
                            style={{ backgroundColor: scrollOffset > 0 ? ts.btn1Bg : ts.outlineBg, color: scrollOffset > 0 ? ts.btnTxt : '#ffffff' }}
                          >
                            ▲
                          </button>
                          <div className="h-[48px] rounded-lg flex items-center justify-center text-[10px] font-mono text-white" style={{ backgroundColor: ts.outlineBg }}>
                            {Math.floor(scrollOffset / 3) + 1}/2
                          </div>
                          <button
                            onClick={() => setScrollOffset(scrollOffset + 3 < 4 ? scrollOffset + 3 : scrollOffset)}
                            className="h-[48px] rounded-lg font-bold text-xs flex items-center justify-center"
                            style={{ backgroundColor: scrollOffset + 3 < 4 ? ts.btn1Bg : ts.outlineBg, color: scrollOffset + 3 < 4 ? ts.btnTxt : '#ffffff' }}
                          >
                            ▼
                          </button>
                        </div>
                      </div>
                    )}

                    {/* 3-Card Vertical Parameter Lists with 1-Line Subtitle Hints + Right-Side Scroll Bar */}
                    {(page === 4 || page === 5 || page === 50 || page === 51 || page === 52) && !showNumpad && (() => {
                      const items = page === 4 ? [0,1,2,3] : page === 5 ? [4,5,21,22] : page === 50 ? [10,17,18,15] : page === 51 ? [6,7,19,20] : [9,14,16];
                      const needsScroll = items.length > 3;
                      const visibleItems = items.slice(scrollOffset, scrollOffset + 3);
                      return (
                        <div className="w-full h-full flex gap-1.5 p-0.5">
                          <div className="flex-1 flex flex-col justify-between gap-1">
                            {visibleItems.map((idx, i) => {
                              const val = settingValues[idx];
                              let valLabel = `${val}${settingUnits[idx]}`;
                              if ((idx === 17 || idx === 18) && val === 0) valLabel = '0 (Def)';
                              else if (idx === 21 && val === 0) valLabel = '0 (Off)';
                              else if (idx === 21 && val > 0) valLabel = `+${val}${settingUnits[idx]}`;
                              return (
                                <button
                                  key={idx}
                                  onClick={() => { setActiveSettingIdx(idx); setNumpadVal(''); setShowNumpad(true); }}
                                  className="h-[48px] w-full rounded-lg px-2.5 flex flex-col justify-center text-left"
                                  style={{ backgroundColor: i % 2 === 0 ? ts.btn1Bg : ts.btn2Bg, color: ts.btnTxt }}
                                >
                                  <div className="w-full flex items-center justify-between font-bold text-[11px] leading-tight">
                                    <span>{settingNames[idx]}:</span>
                                    <span>{valLabel}</span>
                                  </div>
                                  <span className="text-[9px] opacity-80 leading-tight mt-0.5 truncate w-full">
                                    {settingHints[idx]}
                                  </span>
                                </button>
                              );
                            })}
                          </div>
                          {needsScroll && (
                            <div className="w-[42px] flex flex-col justify-between gap-1">
                              <button
                                onClick={() => setScrollOffset(Math.max(0, scrollOffset - 3))}
                                className="h-[48px] rounded-lg font-bold text-xs flex items-center justify-center"
                                style={{ backgroundColor: scrollOffset > 0 ? ts.btn1Bg : ts.outlineBg, color: scrollOffset > 0 ? ts.btnTxt : '#ffffff' }}
                              >
                                ▲
                              </button>
                              <div className="h-[48px] rounded-lg flex items-center justify-center text-[10px] font-mono text-white" style={{ backgroundColor: ts.outlineBg }}>
                                {Math.floor(scrollOffset / 3) + 1}/{Math.ceil(items.length / 3)}
                              </div>
                              <button
                                onClick={() => setScrollOffset(scrollOffset + 3 < items.length ? scrollOffset + 3 : scrollOffset)}
                                className="h-[48px] rounded-lg font-bold text-xs flex items-center justify-center"
                                style={{ backgroundColor: scrollOffset + 3 < items.length ? ts.btn1Bg : ts.outlineBg, color: scrollOffset + 3 < items.length ? ts.btnTxt : '#ffffff' }}
                              >
                                ▼
                              </button>
                            </div>
                          )}
                        </div>
                      );
                    })()}

                    {/* Page 6: Sensor & Scale Settings (with Col. Limit & Scale Cal Wizard button) */}
                    {page === 6 && !showNumpad && (
                      <div className="w-full h-full flex flex-col justify-between p-0.5">
                        {[8, 9, 10].map((idx) => (
                          <div key={idx} className="flex items-center justify-between text-[11px] px-2 py-1 rounded bg-neutral-800/40">
                            <span style={{ color: ts.bannerTxt === '#000000' ? '#fff' : ts.btnTxt }}>{settingNames[idx]}</span>
                            <button
                              onClick={() => { setActiveSettingIdx(idx); setNumpadVal(''); setShowNumpad(true); }}
                              className="px-2.5 py-0.5 rounded font-bold text-[11px] text-white"
                              style={{ backgroundColor: ts.outlineBg }}
                            >
                              {settingValues[idx]} {settingUnits[idx]}
                            </button>
                          </div>
                        ))}
                        <div className="flex items-center justify-between text-[11px] px-2 py-1 rounded bg-neutral-800/40">
                          <span style={{ color: ts.bannerTxt === '#000000' ? '#fff' : ts.btnTxt }}>Load Cell Setup</span>
                          <button
                            onClick={() => {
                              setCalStatus('Step 1: Empty jig & press TARE');
                              setPage(13);
                              setLastEvent('Opened Load Cell Calibration Wizard');
                            }}
                            className="px-3 py-0.5 rounded font-bold text-[11px]"
                            style={{ backgroundColor: ts.btn2Bg, color: ts.btnTxt }}
                          >
                            SCALE CAL &gt;
                          </button>
                        </div>
                      </div>
                    )}

                    {/* Page 13: Guided 2-Step Load Cell Calibration Wizard */}
                    {page === 13 && !showNumpad && (
                      <div className="w-full h-full flex flex-col justify-between p-1">
                        <div className="rounded-lg p-1.5 flex flex-col items-center justify-center text-white" style={{ backgroundColor: ts.outlineBg }}>
                          <div className="text-xs font-bold font-mono">
                            LIVE: {weight.toFixed(1)} g  (Factor: {scaleFactor.toFixed(1)})
                          </div>
                          <div className="text-[10px] text-amber-300 mt-0.5">{calStatus}</div>
                        </div>
                        <button
                          onClick={() => {
                            setWeight(0.0);
                            setCalStatus('Zeroed (0.0g)! Hang known weight & tap 2');
                            setLastEvent('Scale Wizard Step 1: Tared in RAM (10 samples)');
                          }}
                          className="w-full py-2 rounded-lg font-bold text-[11px]"
                          style={{ backgroundColor: ts.btn1Bg, color: ts.btnTxt }}
                        >
                          1. TARE EMPTY JIG (0g)
                        </button>
                        <button
                          onClick={() => {
                            setActiveSettingIdx(23);
                            setNumpadVal('');
                            setShowNumpad(true);
                          }}
                          className="w-full py-2 rounded-lg font-bold text-[11px]"
                          style={{ backgroundColor: ts.btn2Bg, color: ts.btnTxt }}
                        >
                          2. SET KNOWN WEIGHT (g)
                        </button>
                      </div>
                    )}

                    {/* Page 12: Motor Settings */}
                    {page === 12 && !showNumpad && (
                      <div className="w-full h-full flex flex-col justify-between p-1">
                        <div className="flex items-center justify-between text-[11px] px-2 py-1 rounded bg-neutral-800/40">
                          <span style={{ color: ts.bannerTxt === '#000000' ? '#fff' : ts.btnTxt }}>Driver Mode</span>
                          <button
                            onClick={() => {
                              const nextM = (settingValues[12] + 1) % 3;
                              const arr = [...settingValues];
                              arr[12] = nextM;
                              setSettingValues(arr);
                              setLastEvent(`Saved Driver Mode: ${['StealthChop', 'SpreadCycle', 'Adaptive'][nextM]}`);
                            }}
                            className="px-2.5 py-1 rounded font-bold text-[11px]"
                            style={{ backgroundColor: ts.btn1Bg, color: ts.btnTxt }}
                          >
                            {['StealthChop', 'SpreadCycle', 'Adaptive'][settingValues[12]]}
                          </button>
                        </div>
                        <div className="flex items-center justify-between text-[11px] px-2 py-1 rounded bg-neutral-800/40">
                          <span style={{ color: ts.bannerTxt === '#000000' ? '#fff' : ts.btnTxt }}>Hybrid Speed</span>
                          <button
                            onClick={() => { setActiveSettingIdx(13); setNumpadVal(''); setShowNumpad(true); }}
                            className="px-2.5 py-1 rounded font-bold text-[11px] text-white"
                            style={{ backgroundColor: ts.outlineBg }}
                          >
                            {settingValues[13]} mm/s
                          </button>
                        </div>
                        <div className="flex items-center justify-between text-[11px] px-2 py-1 rounded bg-neutral-800/40">
                          <span style={{ color: ts.bannerTxt === '#000000' ? '#fff' : ts.btnTxt }}>Motor Power</span>
                          <button
                            onClick={() => {
                              setMotorEnabled(!motorEnabled);
                              setLastEvent(`Motor coils ${!motorEnabled ? 'ENABLED' : 'DISABLED'}`);
                            }}
                            className="px-2.5 py-1 rounded font-bold text-[11px]"
                            style={{
                              backgroundColor: motorEnabled ? ts.activeBg : '#dc2626',
                              color: motorEnabled ? '#000000' : '#ffffff'
                            }}
                          >
                            {motorEnabled ? 'ENABLED' : 'DISABLED'}
                          </button>
                        </div>
                      </div>
                    )}

                    {/* Page 7: Screen Dashboard */}
                    {page === 7 && (
                      <div className="w-full h-full flex flex-col items-center justify-center gap-2 p-2 text-xs">
                        <span style={{ color: ts.bannerTxt === '#000000' ? '#fff' : ts.btnTxt }}>Brightness</span>
                        <div className="w-full h-5 rounded-full relative overflow-hidden" style={{ backgroundColor: ts.outlineBg }}>
                          <div className="h-full rounded-full" style={{ width: `${brightness}%`, backgroundColor: ts.activeBg }}></div>
                          <span className="absolute inset-0 flex items-center justify-center text-[10px] font-bold text-white">{brightness}%</span>
                        </div>
                        <span style={{ color: ts.bannerTxt === '#000000' ? '#fff' : ts.btnTxt }}>Color Theme (Tap to Change)</span>
                        <button
                          onClick={() => {
                            const nt = (theme + 1) % 3;
                            setTheme(nt);
                            setLastEvent(`Applied Theme: ${['Ind. Slate', 'Bee Mine', 'Midnight'][nt]}`);
                          }}
                          className="w-full py-2 rounded-xl font-bold"
                          style={{ backgroundColor: ts.btn1Bg, color: ts.btnTxt }}
                        >
                          {['Ind. Slate', 'Bee Mine', 'Midnight'][theme]}
                        </button>
                      </div>
                    )}

                    {/* Page 8: Wifi Portal */}
                    {page === 8 && (
                      <div className="w-full h-full flex flex-col items-center justify-center p-2 text-center text-xs">
                        <span className="font-bold mb-1" style={{ color: ts.bannerTxt === '#000000' ? '#fff' : ts.btnTxt }}>WI-FI SETUP PORTAL</span>
                        <div className="w-full p-2 rounded-xl" style={{ backgroundColor: ts.outlineBg }}>
                          <p className="text-white text-[10px]">Connect phone to Wi-Fi:</p>
                          <p className="text-amber-300 font-bold text-xs">BeeMine-Setup</p>
                          <p className="text-white text-[10px]">Open http://192.168.4.1</p>
                        </div>
                      </div>
                    )}

                    {/* Page 9: Active Dip */}
                    {page === 9 && (
                      <div className="w-full h-full flex flex-col items-center justify-center gap-1 p-2 text-center text-xs">
                        <span className="text-amber-400 font-bold">Dip 1: Lowering...</span>
                        <span style={{ color: ts.bannerTxt === '#000000' ? '#fff' : ts.btnTxt }}>Time: 00:04 / 02:45 (est)</span>
                        <span style={{ color: ts.bannerTxt === '#000000' ? '#fff' : ts.btnTxt }}>
                          {activeIsWeight ? `Weight: ${weight.toFixed(0)}g / ${activeIsSlim ? settingValues[0] : settingValues[1]}g` : `Dips: 1 / ${activeIsSlim ? settingValues[2] : settingValues[3]} (${weight.toFixed(0)}g)`}
                        </span>
                        <div className="flex gap-1.5 w-full mt-2">
                          <button
                            onClick={() => {
                              setPosMM(0.0);
                              setCollisionAlert(false);
                              setLowWaxAlert(false);
                              setPage(10);
                              setLastEvent('Paused: Retracted carriage to 0.0mm top rest');
                            }}
                            className="flex-1 py-1.5 rounded-lg border border-red-500 text-red-400 font-bold text-[10px]"
                          >
                            STOP
                          </button>
                          <button
                            onClick={() => {
                              setPosMM(0.0);
                              setCollisionAlert(true);
                              setLowWaxAlert(false);
                              setPage(10);
                              setLastEvent(`Wax Collision (${settingValues[8]}g)! Retracted to 0.0mm top rest!`);
                            }}
                            className="px-2 py-1.5 rounded-lg bg-amber-500/20 border border-amber-500 text-amber-300 font-bold text-[10px]"
                            title="Simulate load cell hitting unmelted wax"
                          >
                            COLLISION
                          </button>
                          <button
                            onClick={() => {
                              setPosMM(0.0);
                              setCollisionAlert(false);
                              setLowWaxAlert(true);
                              setPage(10);
                              setLastEvent(`Low Wax Cutoff at Soft Limit (${settingValues[9]}mm)! Retracted to 0.0mm top rest!`);
                            }}
                            className="px-2 py-1.5 rounded-lg bg-orange-500/20 border border-orange-500 text-orange-300 font-bold text-[10px]"
                            title="Simulate reaching Soft Limit without capacitive sensor triggering"
                          >
                            LOW WAX
                          </button>
                        </div>
                      </div>
                    )}

                    {/* Page 10: Stop Confirm */}
                    {page === 10 && (
                      <div className="w-full h-full flex flex-col items-center justify-center gap-1.5 text-center">
                        {collisionAlert ? (
                          <span className="text-red-400 text-[11px] font-bold">Collision ({settingValues[8]}g)! Raised to top</span>
                        ) : lowWaxAlert ? (
                          <span className="text-orange-400 text-[11px] font-bold">Low Wax! Pot below Soft Limit</span>
                        ) : (
                          <span className="text-amber-300 text-[11px] font-bold">Paused at top rest (0.0mm)</span>
                        )}
                        <span className="text-white text-xs font-bold">Abort or Resume dipping?</span>
                        <div className="flex items-center gap-3 w-full px-4">
                          <button onClick={() => { setCollisionAlert(false); setLowWaxAlert(false); setPage(9); setLastEvent('Resumed dipping from 0.0mm top rest'); }} className="flex-1 py-3 rounded-xl font-bold text-xs text-white" style={{ backgroundColor: ts.outlineBg }}>NO</button>
                          <button 
                            onClick={() => { setDipAborted(true); setCollisionAlert(false); setLowWaxAlert(false); setFinalDurationSecs(45); setPage(11); setLastEvent('Dipping cancelled by user'); }} 
                            className="flex-1 py-3 rounded-xl font-bold text-xs bg-red-600 text-white"
                          >
                            YES
                          </button>
                        </div>
                      </div>
                    )}

                    {/* Page 11: Dip Done */}
                    {page === 11 && (
                      <div className="w-full h-full flex flex-col items-center justify-center gap-2 text-center">
                        {dipAborted ? (
                          <>
                            <span className="text-red-400 text-sm font-bold">
                              {activeIsSlim ? 'Slim cancelled' : 'Std. cancelled'}
                            </span>
                            <span className="text-neutral-300 text-xs font-mono">
                              Cancelled at {Math.floor(finalDurationSecs / 60)}m {finalDurationSecs % 60}s
                            </span>
                            <button 
                              onClick={() => { setPage(0); setDipAborted(false); }} 
                              className="w-full mt-2 py-2.5 rounded-xl font-bold text-xs bg-red-800 text-white"
                            >
                              BACK
                            </button>
                          </>
                        ) : (
                          <>
                            <span className="text-emerald-400 text-sm font-bold">
                              Success!, {activeIsSlim ? 'Slim ready' : 'Std. ready'}
                            </span>
                            <span className="text-neutral-300 text-xs font-mono">
                              Completed in {Math.floor(finalDurationSecs / 60)}m {finalDurationSecs % 60}s
                            </span>
                            <button 
                              onClick={() => { setPage(0); setDipAborted(false); }} 
                              className="w-full mt-2 py-2.5 rounded-xl font-bold text-xs text-black" 
                              style={{ backgroundColor: ts.activeBg }}
                            >
                              FINISH
                            </button>
                          </>
                        )}
                      </div>
                    )}

                    {/* Numpad Overlay */}
                    {showNumpad && (
                      <div className="absolute inset-1 rounded-xl p-2 flex flex-col justify-between" style={{ backgroundColor: ts.bannerBg }}>
                        <div className="text-center font-bold text-xs" style={{ color: ts.bannerTxt }}>{settingNames[activeSettingIdx]}</div>
                        <div className="bg-black text-white px-2 py-1 rounded text-center font-mono font-bold text-xs">{numpadVal || '0'}</div>
                        <div className="grid grid-cols-3 gap-1">
                          {['1','2','3','4','5','6','7','8','9', (activeSettingIdx === 8 || activeSettingIdx === 21) ? '+/-' : 'DEL','0','CLR'].map((k) => (
                            <button
                              key={k}
                              onClick={() => {
                                if (k === 'DEL') setNumpadVal(numpadVal.slice(0, -1));
                                else if (k === 'CLR') setNumpadVal('');
                                else if (k === '+/-') {
                                  if (numpadVal.startsWith('-')) setNumpadVal(numpadVal.slice(1));
                                  else setNumpadVal('-' + numpadVal);
                                }
                                else setNumpadVal(numpadVal + k);
                              }}
                              className="py-1 rounded text-[10px] font-bold text-white"
                              style={{ backgroundColor: k === '+/-' ? '#ca8a04' : ts.outlineBg }}
                            >
                              {k}
                            </button>
                          ))}
                        </div>
                        <div className="flex items-center gap-1 mt-1">
                          <button onClick={() => setShowNumpad(false)} className="flex-1 py-1 rounded bg-red-800 text-white font-bold text-[10px]">CAN</button>
                          <button 
                            onClick={() => {
                              const parsed = parseInt(numpadVal) || 0;
                              if (activeSettingIdx === 23) {
                                if (parsed > 0) {
                                  setWeight(parsed);
                                  setScaleFactor(418.5);
                                  setCalStatus(`Saved! Calibrated to ${parsed}g`);
                                  setLastEvent(`Calibrated Load Cell slope with ${parsed}g reference weight (Saved to NVS)`);
                                }
                              } else {
                                const newArr = [...settingValues];
                                newArr[activeSettingIdx] = parsed;
                                setSettingValues(newArr);
                                setLastEvent(`Updated ${settingNames[activeSettingIdx]} = ${parsed}`);
                              }
                              setShowNumpad(false);
                            }} 
                            className="flex-1 py-1 rounded bg-emerald-700 text-white font-bold text-[10px]"
                          >
                            OK
                          </button>
                        </div>
                      </div>
                    )}
                  </div>

                  {/* Persistent 4-Tab Bottom Bar or Sub-Page Back Button (15% height = 36px) */}
                  {(page < 9 || page >= 12) ? (
                    (page === 0 || page === 2 || page === 14 || page === 3) ? (
                      <div className="h-[36px] w-full grid grid-cols-4 text-[11px] font-bold" style={{ backgroundColor: ts.outlineBg }}>
                        <button
                          onClick={() => { setPage(0); setLastEvent('Switched to DIP tab'); }}
                          className="h-full flex items-center justify-center border-r border-black/30"
                          style={{
                            backgroundColor: page === 0 ? ts.bannerBg : ts.outlineBg,
                            color: page === 0 ? ts.bannerTxt : '#ffffff'
                          }}
                        >
                          DIP
                        </button>
                        <button
                          onClick={() => { setPage(2); setLastEvent('Switched to MANUAL tab'); }}
                          className="h-full flex items-center justify-center border-r border-black/30"
                          style={{
                            backgroundColor: page === 2 ? ts.bannerBg : ts.outlineBg,
                            color: page === 2 ? ts.bannerTxt : '#ffffff'
                          }}
                        >
                          MANUAL
                        </button>
                        <button
                          onClick={() => { setPage(14); setLastEvent('Switched to TOOLS tab'); }}
                          className="h-full flex items-center justify-center border-r border-black/30"
                          style={{
                            backgroundColor: page === 14 ? ts.bannerBg : ts.outlineBg,
                            color: page === 14 ? ts.bannerTxt : '#ffffff'
                          }}
                        >
                          TOOLS
                        </button>
                        <button
                          onClick={() => { setScrollOffset(0); setPage(3); setLastEvent('Switched to SETUP tab (2-Bucket Hub)'); }}
                          className="h-full flex items-center justify-center"
                          style={{
                            backgroundColor: page === 3 ? ts.bannerBg : ts.outlineBg,
                            color: page === 3 ? ts.bannerTxt : '#ffffff'
                          }}
                        >
                          SETUP
                        </button>
                      </div>
                    ) : (
                      <div
                        onClick={() => {
                          setScrollOffset(0);
                          if (showNumpad) setShowNumpad(false);
                          else if (page === 13) { setPage(6); setLastEvent('Returned to Scale & Col Settings'); }
                          else if (page === 30 || page === 31) { setPage(3); setLastEvent('Returned to 2-Bucket Setup Hub'); }
                          else if (page === 6 || page === 12 || page === 7 || page === 8 || page === 52 || page === 53) { setPage(31); setLastEvent('Returned to 2. MACHINE SETUP'); }
                          else { setPage(30); setLastEvent('Returned to 1. RECIPE PARAMS'); }
                        }}
                        className="h-[36px] w-full flex items-center justify-center text-xs font-bold cursor-pointer text-white"
                        style={{ backgroundColor: ts.outlineBg }}
                      >
                        {page === 13 ? '< BACK TO SENSOR' : (page === 30 || page === 31) ? '< BACK TO SETUP' : (page === 6 || page === 12 || page === 7 || page === 8 || page === 52 || page === 53) ? '< BACK TO MACHINE' : '< BACK TO RECIPE'}
                      </div>
                    )
                  ) : null}
                </div>

                {/* Touch Simulation Help Footer */}
                <div className="mt-4 pt-3 border-t border-neutral-700/50 flex items-center justify-between text-xs text-neutral-400 font-mono">
                  <span>Last Event:</span>
                  <span className="text-amber-400 font-sans truncate max-w-[220px]">{lastEvent}</span>
                </div>
              </div>
            </div>

            {/* Side Control & Event Console */}
            <div className="lg:col-span-5 flex flex-col gap-4">
              <div className="bg-neutral-950 border border-neutral-800 rounded-xl p-5">
                <h3 className="text-sm font-semibold text-white mb-3 flex items-center gap-2">
                  <Sliders className="w-4 h-4 text-amber-400" />
                  M5Canvas DisplayManager Test Bench
                </h3>
                <p className="text-xs text-neutral-400 mb-4 leading-relaxed">
                  Test the 4-Tab <code className="text-amber-300 font-mono">DisplayManager</code>, the <strong>2-Bucket Setup Hub</strong> (`1. RECIPE PARAMS` vs `2. MACHINE SETUP`), and the <strong>3-Card Vertical Scroll UI</strong> with subtitle hints.
                </p>

                <div className="grid grid-cols-2 gap-2 mb-4">
                  <button
                    onClick={() => { setPage(0); setShowNumpad(false); }}
                    className="flex items-center justify-center gap-2 px-3 py-2 rounded-lg bg-neutral-900 border border-neutral-800 text-xs font-medium hover:border-amber-400/50 text-neutral-200"
                  >
                    <Scale className="w-3.5 h-3.5 text-amber-400" />
                    Dip Launcher
                  </button>
                  <button
                    onClick={() => { setPage(14); setShowNumpad(false); }}
                    className="flex items-center justify-center gap-2 px-3 py-2 rounded-lg bg-neutral-900 border border-neutral-800 text-xs font-medium hover:border-amber-400/50 text-neutral-200"
                  >
                    <Sparkles className="w-3.5 h-3.5 text-emerald-400" />
                    Jig &amp; Base Tools
                  </button>
                  <button
                    onClick={() => { setScrollOffset(0); setPage(30); setShowNumpad(false); }}
                    className="flex items-center justify-center gap-2 px-3 py-2 rounded-lg bg-neutral-900 border border-neutral-800 text-xs font-medium hover:border-amber-400/50 text-neutral-200"
                  >
                    <Sliders className="w-3.5 h-3.5 text-blue-400" />
                    1. Recipe Params
                  </button>
                  <button
                    onClick={() => { setScrollOffset(0); setPage(31); setShowNumpad(false); }}
                    className="flex items-center justify-center gap-2 px-3 py-2 rounded-lg bg-neutral-900 border border-neutral-800 text-xs font-medium hover:border-amber-400/50 text-neutral-200"
                  >
                    <Settings className="w-3.5 h-3.5 text-purple-400" />
                    2. Machine Setup
                  </button>
                </div>

                <div className="space-y-3 pt-3 border-t border-neutral-800">
                  <div>
                    <label className="text-xs text-neutral-400 block mb-1">Upgraded Color Theme Palettes</label>
                    <div className="grid grid-cols-3 gap-2">
                      {['Ind. Slate', 'Bee Mine', 'Midnight'].map((tName, i) => (
                        <button
                          key={tName}
                          onClick={() => { setTheme(i); setLastEvent(`Applied Theme: ${tName}`); }}
                          className={`py-1.5 rounded text-xs font-medium border ${
                            theme === i 
                              ? 'bg-amber-400/10 border-amber-400 text-amber-300' 
                              : 'bg-neutral-900 border-neutral-800 text-neutral-400 hover:text-white'
                          }`}
                        >
                          {tName}
                        </button>
                      ))}
                    </div>
                  </div>

                  <div>
                    <div className="flex justify-between text-xs text-neutral-400 mb-1">
                      <span>Screen Brightness PWM</span>
                      <span className="font-mono text-amber-400">{brightness}%</span>
                    </div>
                    <input
                      type="range"
                      min="1"
                      max="99"
                      value={brightness}
                      onChange={(e) => setBrightness(parseInt(e.target.value))}
                      className="w-full accent-amber-400 bg-neutral-800 rounded h-1.5"
                    />
                  </div>
                </div>
              </div>

              {/* Verification Status */}
              <div className="bg-emerald-950/30 border border-emerald-800/40 rounded-xl p-4">
                <div className="flex items-center gap-2 text-emerald-400 font-semibold text-xs mb-2">
                  <CheckCircle2 className="w-4 h-4" />
                  Latest Firmware Enhancements
                </div>
                <ul className="text-xs text-neutral-300 space-y-1.5 list-disc list-inside leading-relaxed">
                  <li><strong>2-Bucket Setup &amp; 3-Card Scroll UI:</strong> Splits settings into <code className="text-emerald-300 font-mono">1. RECIPE PARAMS</code> and <code className="text-emerald-300 font-mono">2. MACHINE SETUP</code> with 3 tall cards + subtitle hints and a right-hand <code className="text-emerald-300 font-mono">▲ / ▼</code> scroll column.</li>
                  <li><strong>Upgraded Color Palettes:</strong> Refined <code className="text-emerald-300 font-mono">Ind. Slate</code>, <code className="text-emerald-300 font-mono">Bee Mine</code> (Espresso, Honeycomb &amp; Sage), and <code className="text-emerald-300 font-mono">Midnight</code> OLED themes.</li>
                  <li><strong>Kerzenkiste 36-Candle Jig Manual:</strong> Full print-ready <code className="text-emerald-300 font-mono">USER_MANUAL.md</code> with Executive Summary, Index, Speed Glossary, and Detailed Reference.</li>
                </ul>
              </div>
            </div>
          </div>
        )}

        {activeTab === 'manual' && (
          <div className="bg-neutral-950 border border-neutral-800 rounded-xl overflow-hidden flex flex-col">
            <div className="px-5 py-3 border-b border-neutral-800 flex items-center justify-between bg-neutral-900/50">
              <div className="flex items-center gap-2">
                <Package className="w-4 h-4 text-amber-400" />
                <span className="text-xs font-mono font-semibold text-white">/USER_MANUAL.md</span>
                <span className="text-[10px] px-2 py-0.5 rounded bg-emerald-500/10 text-emerald-400 border border-emerald-500/20">
                  Print-Ready Operations &amp; Parameter Manual
                </span>
              </div>
              <div className="flex items-center gap-2">
                <button
                  onClick={() => downloadSingleFile('USER_MANUAL.md', userManualRaw)}
                  className="flex items-center gap-1.5 px-3 py-1 rounded bg-neutral-800 hover:bg-neutral-700 text-xs font-medium text-neutral-200 transition-all cursor-pointer"
                >
                  <Download className="w-3.5 h-3.5 text-amber-400" />
                  Download
                </button>
                <button
                  onClick={() => copyCode(userManualRaw, 'manual')}
                  className="flex items-center gap-1.5 px-3 py-1 rounded bg-neutral-800 hover:bg-neutral-700 text-xs font-medium text-neutral-200 transition-all cursor-pointer"
                >
                  {copiedFile === 'manual' ? <Check className="w-3.5 h-3.5 text-emerald-400" /> : <Copy className="w-3.5 h-3.5" />}
                  {copiedFile === 'manual' ? 'Copied!' : 'Copy'}
                </button>
              </div>
            </div>
            <pre className="p-5 font-mono text-xs text-neutral-300 overflow-x-auto leading-relaxed bg-neutral-950 max-h-[600px] overflow-y-auto whitespace-pre-wrap">
              {userManualRaw}
            </pre>
          </div>
        )}

        {activeTab === 'main' && (
          <div className="bg-neutral-950 border border-neutral-800 rounded-xl overflow-hidden flex flex-col">
            <div className="px-5 py-3 border-b border-neutral-800 flex items-center justify-between bg-neutral-900/50">
              <div className="flex items-center gap-2">
                <Code2 className="w-4 h-4 text-amber-400" />
                <span className="text-xs font-mono font-semibold text-white">/src/main.cpp</span>
              </div>
              <button
                onClick={() => copyCode(mainCppRaw, 'main')}
                className="flex items-center gap-1.5 px-3 py-1 rounded bg-neutral-800 hover:bg-neutral-700 text-xs font-medium text-neutral-200 transition-all"
              >
                {copiedFile === 'main' ? <Check className="w-3.5 h-3.5 text-emerald-400" /> : <Copy className="w-3.5 h-3.5" />}
                {copiedFile === 'main' ? 'Copied main.cpp!' : 'Copy main.cpp'}
              </button>
            </div>
            <pre className="p-5 font-mono text-xs text-neutral-300 overflow-x-auto leading-relaxed bg-neutral-950 max-h-[600px] overflow-y-auto">
              {mainCppRaw}
            </pre>
          </div>
        )}

        {activeTab === 'motor' && (
          <div className="bg-neutral-950 border border-neutral-800 rounded-xl overflow-hidden flex flex-col">
            <div className="px-5 py-3 border-b border-neutral-800 flex items-center justify-between bg-neutral-900/50">
              <div className="flex items-center gap-2">
                <Cpu className="w-4 h-4 text-amber-400" />
                <span className="text-xs font-mono font-semibold text-white">/src/MotorManager.h</span>
              </div>
              <button
                onClick={() => copyCode(motorManagerHRaw, 'motor')}
                className="flex items-center gap-1.5 px-3 py-1 rounded bg-neutral-800 hover:bg-neutral-700 text-xs font-medium text-neutral-200 transition-all"
              >
                {copiedFile === 'motor' ? <Check className="w-3.5 h-3.5 text-emerald-400" /> : <Copy className="w-3.5 h-3.5" />}
                {copiedFile === 'motor' ? 'Copied MotorManager.h!' : 'Copy MotorManager.h'}
              </button>
            </div>
            <pre className="p-5 font-mono text-xs text-neutral-300 overflow-x-auto leading-relaxed bg-neutral-950 max-h-[600px] overflow-y-auto">
              {motorManagerHRaw}
            </pre>
          </div>
        )}

        {activeTab === 'motorcpp' && (
          <div className="bg-neutral-950 border border-neutral-800 rounded-xl overflow-hidden flex flex-col">
            <div className="px-5 py-3 border-b border-neutral-800 flex items-center justify-between bg-neutral-900/50">
              <div className="flex items-center gap-2">
                <Cpu className="w-4 h-4 text-amber-400" />
                <span className="text-xs font-mono font-semibold text-white">/src/MotorManager.cpp</span>
              </div>
              <button
                onClick={() => copyCode(motorManagerCppRaw, 'motorcpp')}
                className="flex items-center gap-1.5 px-3 py-1 rounded bg-neutral-800 hover:bg-neutral-700 text-xs font-medium text-neutral-200 transition-all"
              >
                {copiedFile === 'motorcpp' ? <Check className="w-3.5 h-3.5 text-emerald-400" /> : <Copy className="w-3.5 h-3.5" />}
                {copiedFile === 'motorcpp' ? 'Copied MotorManager.cpp!' : 'Copy MotorManager.cpp'}
              </button>
            </div>
            <pre className="p-5 font-mono text-xs text-neutral-300 overflow-x-auto leading-relaxed bg-neutral-950 max-h-[600px] overflow-y-auto">
              {motorManagerCppRaw}
            </pre>
          </div>
        )}

        {activeTab === 'scale' && (
          <div className="bg-neutral-950 border border-neutral-800 rounded-xl overflow-hidden flex flex-col">
            <div className="px-5 py-3 border-b border-neutral-800 flex items-center justify-between bg-neutral-900/50">
              <div className="flex items-center gap-2">
                <Scale className="w-4 h-4 text-amber-400" />
                <span className="text-xs font-mono font-semibold text-white">/src/ScaleManager.h</span>
              </div>
              <button
                onClick={() => copyCode(scaleManagerHRaw, 'scale')}
                className="flex items-center gap-1.5 px-3 py-1 rounded bg-neutral-800 hover:bg-neutral-700 text-xs font-medium text-neutral-200 transition-all"
              >
                {copiedFile === 'scale' ? <Check className="w-3.5 h-3.5 text-emerald-400" /> : <Copy className="w-3.5 h-3.5" />}
                {copiedFile === 'scale' ? 'Copied ScaleManager.h!' : 'Copy ScaleManager.h'}
              </button>
            </div>
            <pre className="p-5 font-mono text-xs text-neutral-300 overflow-x-auto leading-relaxed bg-neutral-950 max-h-[600px] overflow-y-auto">
              {scaleManagerHRaw}
            </pre>
          </div>
        )}

        {activeTab === 'scalecpp' && (
          <div className="bg-neutral-950 border border-neutral-800 rounded-xl overflow-hidden flex flex-col">
            <div className="px-5 py-3 border-b border-neutral-800 flex items-center justify-between bg-neutral-900/50">
              <div className="flex items-center gap-2">
                <Scale className="w-4 h-4 text-amber-400" />
                <span className="text-xs font-mono font-semibold text-white">/src/ScaleManager.cpp</span>
              </div>
              <button
                onClick={() => copyCode(scaleManagerCppRaw, 'scalecpp')}
                className="flex items-center gap-1.5 px-3 py-1 rounded bg-neutral-800 hover:bg-neutral-700 text-xs font-medium text-neutral-200 transition-all"
              >
                {copiedFile === 'scalecpp' ? <Check className="w-3.5 h-3.5 text-emerald-400" /> : <Copy className="w-3.5 h-3.5" />}
                {copiedFile === 'scalecpp' ? 'Copied ScaleManager.cpp!' : 'Copy ScaleManager.cpp'}
              </button>
            </div>
            <pre className="p-5 font-mono text-xs text-neutral-300 overflow-x-auto leading-relaxed bg-neutral-950 max-h-[600px] overflow-y-auto">
              {scaleManagerCppRaw}
            </pre>
          </div>
        )}

        {activeTab === 'sensor' && (
          <div className="bg-neutral-950 border border-neutral-800 rounded-xl overflow-hidden flex flex-col">
            <div className="px-5 py-3 border-b border-neutral-800 flex items-center justify-between bg-neutral-900/50">
              <div className="flex items-center gap-2">
                <Sliders className="w-4 h-4 text-amber-400" />
                <span className="text-xs font-mono font-semibold text-white">/src/SensorManager.h</span>
              </div>
              <button
                onClick={() => copyCode(sensorManagerHRaw, 'sensor')}
                className="flex items-center gap-1.5 px-3 py-1 rounded bg-neutral-800 hover:bg-neutral-700 text-xs font-medium text-neutral-200 transition-all"
              >
                {copiedFile === 'sensor' ? <Check className="w-3.5 h-3.5 text-emerald-400" /> : <Copy className="w-3.5 h-3.5" />}
                {copiedFile === 'sensor' ? 'Copied SensorManager.h!' : 'Copy SensorManager.h'}
              </button>
            </div>
            <pre className="p-5 font-mono text-xs text-neutral-300 overflow-x-auto leading-relaxed bg-neutral-950 max-h-[600px] overflow-y-auto">
              {sensorManagerHRaw}
            </pre>
          </div>
        )}

        {activeTab === 'sound' && (
          <div className="bg-neutral-950 border border-neutral-800 rounded-xl overflow-hidden flex flex-col">
            <div className="px-5 py-3 border-b border-neutral-800 flex items-center justify-between bg-neutral-900/50">
              <div className="flex items-center gap-2">
                <Code2 className="w-4 h-4 text-amber-400" />
                <span className="text-xs font-mono font-semibold text-white">/src/SoundManager.h</span>
              </div>
              <button
                onClick={() => copyCode(soundManagerHRaw, 'sound')}
                className="flex items-center gap-1.5 px-3 py-1 rounded bg-neutral-800 hover:bg-neutral-700 text-xs font-medium text-neutral-200 transition-all"
              >
                {copiedFile === 'sound' ? <Check className="w-3.5 h-3.5 text-emerald-400" /> : <Copy className="w-3.5 h-3.5" />}
                {copiedFile === 'sound' ? 'Copied SoundManager.h!' : 'Copy SoundManager.h'}
              </button>
            </div>
            <pre className="p-5 font-mono text-xs text-neutral-300 overflow-x-auto leading-relaxed bg-neutral-950 max-h-[600px] overflow-y-auto">
              {soundManagerHRaw}
            </pre>
          </div>
        )}

        {activeTab === 'soundcpp' && (
          <div className="bg-neutral-950 border border-neutral-800 rounded-xl overflow-hidden flex flex-col">
            <div className="px-5 py-3 border-b border-neutral-800 flex items-center justify-between bg-neutral-900/50">
              <div className="flex items-center gap-2">
                <Code2 className="w-4 h-4 text-amber-400" />
                <span className="text-xs font-mono font-semibold text-white">/src/SoundManager.cpp</span>
              </div>
              <button
                onClick={() => copyCode(soundManagerCppRaw, 'soundcpp')}
                className="flex items-center gap-1.5 px-3 py-1 rounded bg-neutral-800 hover:bg-neutral-700 text-xs font-medium text-neutral-200 transition-all"
              >
                {copiedFile === 'soundcpp' ? <Check className="w-3.5 h-3.5 text-emerald-400" /> : <Copy className="w-3.5 h-3.5" />}
                {copiedFile === 'soundcpp' ? 'Copied SoundManager.cpp!' : 'Copy SoundManager.cpp'}
              </button>
            </div>
            <pre className="p-5 font-mono text-xs text-neutral-300 overflow-x-auto leading-relaxed bg-neutral-950 max-h-[600px] overflow-y-auto">
              {soundManagerCppRaw}
            </pre>
          </div>
        )}

        {activeTab === 'header' && (
          <div className="bg-neutral-950 border border-neutral-800 rounded-xl overflow-hidden flex flex-col">
            <div className="px-5 py-3 border-b border-neutral-800 flex items-center justify-between bg-neutral-900/50">
              <div className="flex items-center gap-2">
                <Code2 className="w-4 h-4 text-amber-400" />
                <span className="text-xs font-mono font-semibold text-white">/src/DisplayManager.h</span>
              </div>
              <button
                onClick={() => copyCode(displayManagerHRaw, 'header')}
                className="flex items-center gap-1.5 px-3 py-1 rounded bg-neutral-800 hover:bg-neutral-700 text-xs font-medium text-neutral-200 transition-all"
              >
                {copiedFile === 'header' ? <Check className="w-3.5 h-3.5 text-emerald-400" /> : <Copy className="w-3.5 h-3.5" />}
                {copiedFile === 'header' ? 'Copied Header!' : 'Copy DisplayManager.h'}
              </button>
            </div>
            <pre className="p-5 font-mono text-xs text-neutral-300 overflow-x-auto leading-relaxed bg-neutral-950 max-h-[600px] overflow-y-auto">
              {displayManagerHRaw}
            </pre>
          </div>
        )}

        {activeTab === 'cpp' && (
          <div className="bg-neutral-950 border border-neutral-800 rounded-xl overflow-hidden flex flex-col">
            <div className="px-5 py-3 border-b border-neutral-800 flex items-center justify-between bg-neutral-900/50">
              <div className="flex items-center gap-2">
                <Cpu className="w-4 h-4 text-amber-400" />
                <span className="text-xs font-mono font-semibold text-white">/src/DisplayManager.cpp</span>
              </div>
              <button
                onClick={() => copyCode(displayManagerCppRaw, 'cpp')}
                className="flex items-center gap-1.5 px-3 py-1 rounded bg-neutral-800 hover:bg-neutral-700 text-xs font-medium text-neutral-200 transition-all"
              >
                {copiedFile === 'cpp' ? <Check className="w-3.5 h-3.5 text-emerald-400" /> : <Copy className="w-3.5 h-3.5" />}
                {copiedFile === 'cpp' ? 'Copied Implementation!' : 'Copy DisplayManager.cpp'}
              </button>
            </div>
            <pre className="p-5 font-mono text-xs text-neutral-300 overflow-x-auto leading-relaxed bg-neutral-950 max-h-[600px] overflow-y-auto">
              {displayManagerCppRaw}
            </pre>
          </div>
        )}

        {activeTab === 'pinconfig' && (
          <div className="bg-neutral-950 border border-neutral-800 rounded-xl overflow-hidden flex flex-col">
            <div className="px-5 py-3 border-b border-neutral-800 flex items-center justify-between bg-neutral-900/50">
              <div className="flex items-center gap-2">
                <Settings className="w-4 h-4 text-amber-400" />
                <span className="text-xs font-mono font-semibold text-white">/src/PinConfig.h</span>
              </div>
              <div className="flex items-center gap-2">
                <button
                  onClick={() => downloadSingleFile('PinConfig.h', pinConfigRaw)}
                  className="flex items-center gap-1.5 px-3 py-1 rounded bg-neutral-800 hover:bg-neutral-700 text-xs font-medium text-neutral-200 transition-all cursor-pointer"
                >
                  <Download className="w-3.5 h-3.5 text-amber-400" />
                  Download
                </button>
                <button
                  onClick={() => copyCode(pinConfigRaw, 'pinconfig')}
                  className="flex items-center gap-1.5 px-3 py-1 rounded bg-neutral-800 hover:bg-neutral-700 text-xs font-medium text-neutral-200 transition-all cursor-pointer"
                >
                  {copiedFile === 'pinconfig' ? <Check className="w-3.5 h-3.5 text-emerald-400" /> : <Copy className="w-3.5 h-3.5" />}
                  {copiedFile === 'pinconfig' ? 'Copied!' : 'Copy'}
                </button>
              </div>
            </div>
            <pre className="p-5 font-mono text-xs text-neutral-300 overflow-x-auto leading-relaxed bg-neutral-950 max-h-[600px] overflow-y-auto">
              {pinConfigRaw}
            </pre>
          </div>
        )}

        {activeTab === 'sensorcpp' && (
          <div className="bg-neutral-950 border border-neutral-800 rounded-xl overflow-hidden flex flex-col">
            <div className="px-5 py-3 border-b border-neutral-800 flex items-center justify-between bg-neutral-900/50">
              <div className="flex items-center gap-2">
                <Sliders className="w-4 h-4 text-amber-400" />
                <span className="text-xs font-mono font-semibold text-white">/src/SensorManager.cpp</span>
                <span className="text-[10px] px-2 py-0.5 rounded bg-emerald-500/10 text-emerald-400 border border-emerald-500/20">
                  PC817 Active-LOW Verified
                </span>
              </div>
              <div className="flex items-center gap-2">
                <button
                  onClick={() => downloadSingleFile('SensorManager.cpp', sensorManagerCppRaw)}
                  className="flex items-center gap-1.5 px-3 py-1 rounded bg-neutral-800 hover:bg-neutral-700 text-xs font-medium text-neutral-200 transition-all cursor-pointer"
                >
                  <Download className="w-3.5 h-3.5 text-amber-400" />
                  Download
                </button>
                <button
                  onClick={() => copyCode(sensorManagerCppRaw, 'sensorcpp')}
                  className="flex items-center gap-1.5 px-3 py-1 rounded bg-neutral-800 hover:bg-neutral-700 text-xs font-medium text-neutral-200 transition-all cursor-pointer"
                >
                  {copiedFile === 'sensorcpp' ? <Check className="w-3.5 h-3.5 text-emerald-400" /> : <Copy className="w-3.5 h-3.5" />}
                  {copiedFile === 'sensorcpp' ? 'Copied!' : 'Copy'}
                </button>
              </div>
            </div>
            <pre className="p-5 font-mono text-xs text-neutral-300 overflow-x-auto leading-relaxed bg-neutral-950 max-h-[600px] overflow-y-auto">
              {sensorManagerCppRaw}
            </pre>
          </div>
        )}

        {activeTab === 'platformio' && (
          <div className="bg-neutral-950 border border-neutral-800 rounded-xl overflow-hidden flex flex-col">
            <div className="px-5 py-3 border-b border-neutral-800 flex items-center justify-between bg-neutral-900/50">
              <div className="flex items-center gap-2">
                <FileCode className="w-4 h-4 text-amber-400" />
                <span className="text-xs font-mono font-semibold text-white">/platformio.ini</span>
              </div>
              <div className="flex items-center gap-2">
                <button
                  onClick={() => downloadSingleFile('platformio.ini', platformioIniRaw)}
                  className="flex items-center gap-1.5 px-3 py-1 rounded bg-neutral-800 hover:bg-neutral-700 text-xs font-medium text-neutral-200 transition-all cursor-pointer"
                >
                  <Download className="w-3.5 h-3.5 text-amber-400" />
                  Download
                </button>
                <button
                  onClick={() => copyCode(platformioIniRaw, 'platformio')}
                  className="flex items-center gap-1.5 px-3 py-1 rounded bg-neutral-800 hover:bg-neutral-700 text-xs font-medium text-neutral-200 transition-all cursor-pointer"
                >
                  {copiedFile === 'platformio' ? <Check className="w-3.5 h-3.5 text-emerald-400" /> : <Copy className="w-3.5 h-3.5" />}
                  {copiedFile === 'platformio' ? 'Copied!' : 'Copy'}
                </button>
              </div>
            </div>
            <pre className="p-5 font-mono text-xs text-neutral-300 overflow-x-auto leading-relaxed bg-neutral-950 max-h-[600px] overflow-y-auto">
              {platformioIniRaw}
            </pre>
          </div>
        )}

        {activeTab === 'readme' && (
          <div className="bg-neutral-950 border border-neutral-800 rounded-xl overflow-hidden flex flex-col">
            <div className="px-5 py-3 border-b border-neutral-800 flex items-center justify-between bg-neutral-900/50">
              <div className="flex items-center gap-2">
                <Package className="w-4 h-4 text-amber-400" />
                <span className="text-xs font-mono font-semibold text-white">/README.md</span>
                <span className="text-[10px] px-2 py-0.5 rounded bg-amber-500/10 text-amber-400 border border-amber-500/20">
                  Antigravity Documentation & Pinout
                </span>
              </div>
              <div className="flex items-center gap-2">
                <button
                  onClick={() => downloadSingleFile('README.md', readmeRaw)}
                  className="flex items-center gap-1.5 px-3 py-1 rounded bg-neutral-800 hover:bg-neutral-700 text-xs font-medium text-neutral-200 transition-all cursor-pointer"
                >
                  <Download className="w-3.5 h-3.5 text-amber-400" />
                  Download
                </button>
                <button
                  onClick={() => copyCode(readmeRaw, 'readme')}
                  className="flex items-center gap-1.5 px-3 py-1 rounded bg-neutral-800 hover:bg-neutral-700 text-xs font-medium text-neutral-200 transition-all cursor-pointer"
                >
                  {copiedFile === 'readme' ? <Check className="w-3.5 h-3.5 text-emerald-400" /> : <Copy className="w-3.5 h-3.5" />}
                  {copiedFile === 'readme' ? 'Copied!' : 'Copy'}
                </button>
              </div>
            </div>
            <pre className="p-5 font-mono text-xs text-neutral-300 overflow-x-auto leading-relaxed bg-neutral-950 max-h-[600px] overflow-y-auto whitespace-pre-wrap">
              {readmeRaw}
            </pre>
          </div>
        )}

        {activeTab === 'architecture' && (
          <div className="space-y-6">
            <div className="bg-neutral-950 border border-neutral-800 rounded-xl p-6">
              <h2 className="text-base font-semibold text-white mb-2 flex items-center gap-2">
                <Sparkles className="w-5 h-5 text-amber-400" />
                Refactoring & Hardware Integration - Phase 2
              </h2>
              <p className="text-xs text-neutral-400 leading-relaxed">
                The firmware cleanly separates UI rendering, touch hit-testing, stepper motor control, sensor debouncing, and 24-bit load cell calibration across dedicated C++ managers.
              </p>
            </div>
          </div>
        )}

      </div>
    </div>
  );
}
