import subprocess
import time
from pathlib import Path
from system_monitor import LinuxMonitor
import argparse
import sys

class ProgressBar:
    
    @staticmethod
    def show(iteration, total, prefix='', suffix='', length=50, fill='X'):
        percent = ("{0:.1f}").format(100 * (iteration / float(total)))
        filled_length = int(length * iteration // total)
        bar = fill * filled_length + 'o' * (length - filled_length)
        
        sys.stdout.write(f'\r{prefix} |{bar}| {percent}% {suffix}')
        sys.stdout.flush()
        
        if iteration == total: 
            sys.stdout.write('\n')
    
    @staticmethod
    def animate_loading(text="loading", duration=1):
        frames = ["⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"]
        
        end_time = time.time() + duration
        i = 0
        
        while time.time() < end_time:
            sys.stdout.write(f"\r{text} {frames[i % len(frames)]}")
            sys.stdout.flush()
            time.sleep(0.1)
            i += 1
        
        sys.stdout.write(f"\r{text} ✅\n")
        sys.stdout.flush()

class SimpleController:
    def __init__(self, threads=4):
        self.threads = threads
        self.base_dir = Path(__file__).parent.parent
        self.build_dir = self.base_dir / "build"
        self.results_dir = self.base_dir / "results"
        
        self.build_dir.mkdir(exist_ok=True)
        self.results_dir.mkdir(exist_ok=True)
        
        self.demos = [
            {
                'name': 'Race Condition Demo',
                'program': self.build_dir / 'race_demo',
                'args' : [str(self.threads)]
            },
            {
                'name': 'Deadlock Demo',
                'program': self.build_dir / 'deadlock_demo',
                'args' : [str(self.threads)]
            },
            {
                'name': 'Normal mode',
                'program': self.build_dir / 'normal',
                'args': [str(self.threads)]
            }
        ]
    
    def compile_cpp(self):
        print(f"Compile .cpp files for {self.threads} threads")
        
        files_to_compile = [
            ('race_demo.cpp', 'race_demo'),
            ('deadlock_demo.cpp', 'deadlock_demo'),
            ('normal.cpp', 'normal')
        ]
        
        total_files = len(files_to_compile)
        
        for i, (source, target) in enumerate(files_to_compile, 1):
            source_path = self.base_dir / "cpp" / source
            target_path = self.build_dir / target
            
            ProgressBar.show(i, total_files, prefix='Progress:', suffix=f'{source} -> {target}\n')
            
            cmd = [
                'g++',
                '-std=c++17',
                '-pthread',
                '-O2',
                '-o',
                str(target_path),
                str(source_path)
            ]
            
            try:
                result = subprocess.run(cmd, capture_output=True, text=True)
                
                if result.returncode == 0:
                    print(f"Successfully: {target}")
                else:
                    print(f"Compile error in {target}:")
                    print(result.stderr)
                
            except Exception as e:
                print(f"Exception from {target}: {e}")
                
    def run_single_demo(self, demo, demo_num, total_demo):
        print(f"\n=== Running: {demo['name']} ===")
    
        if not demo['program'].exists():
            print("Executable not found")
            return None
        
        ProgressBar.animate_loading(f"   Starting {demo['name']}", duration=1)
        
        monitor = LinuxMonitor(0)
        
        timeout = 5 if "Deadlock" in demo['name'] else 15
        
        result = monitor.measure_with_perf([str(demo['program'])] + demo['args'], timeout=15)
        
        summary = monitor.get_summary()
        
        print(f"\nResults:")
        print("-"*30)
        print(f"Wall time: {summary['wall_time']:.3f}s")
        print(f"CPU time: {summary['cpu_time']:.3f}s")
        
        if 'cpus_utilized' in summary:
            print(f"CPUs utilized: {summary['cpus_utilized']:.2f} cores")
            
            if 'cpu_percent_single_core' in summary:
                print(f"CPU load: {summary['cpu_percent_single_core']:.1f}% (of single core)")
        
            if 'cpu_percent_total' in summary and 'system_cores' in summary:
                print(f"System usage: {summary['cpu_percent_total']:.1f}% of {summary['system_cores']} cores")
        
        if 'parallelism' in summary:
            print(f"Parallelism: {summary['parallelism']:.2f}x")

        if 'max_threads' in summary:
            print(f"Max threads: {summary['max_threads']}")
            
        print("-"*30)
        
        output = result['program_output'] + result['program_stderr']
        
        if "DATA RACE" in output:
            print(f"Data race detected")
        if "DEADLOCK" in output:
            print(f"Deadlock detected")
        if result['return_code'] == -1:
            print(f"Timeout - possible deadlock")
        
        return {
            'name': demo['name'],
            'exit_code': result['return_code'],
            'stdout': result['program_output'],
            'stderr': result['program_stderr'],
            'metrics': summary
        }
        
    def run_all_demos(self):
        print("Run all demos")
        
        results = []
        total_demos = len(self.demos)
        
        for i, demo in enumerate(self.demos, 1):
            ProgressBar.show(i, total_demos, prefix='[System] Progress:', suffix=f'Demo {i}/{total_demos}')
            
            result = self.run_single_demo(demo, i, total_demos)
            if result:
                results.append(result)
                
            if i < total_demos:
                time.sleep(1)
            
        return results
    
    def generate_report(self, results):
        print("=== Report ===")
        
        report_path = self.results_dir / "report.txt"
        
        with open(report_path, 'w', encoding='utf-8') as f:
            f.write("Report\n")
            f.write("Theme: Threads\n")
            f.write(f"Date: {time.strftime('%Y-%m-%d %H:%M:%S')}\n")
            f.write("="*50 + "\n\n")
            
            for result in results:
                f.write(f"Demo: {result['name']}\n")
                f.write(f"Status: {'SUCCESS' if result['exit_code'] == 0 else 'ERROR'}\n")
                
                metrics = result.get('metrics', {})
                
                if 'wall_time' in metrics:
                    f.write(f"Wall time: {metrics['wall_time']:.3f}s\n")
                
                if 'cpu_time' in metrics:
                    f.write(f"CPU time: {metrics['cpu_time']:.3f}s\n")
                
                if 'cpus_utilized' in metrics:
                    f.write(f"CPUs utilized: {metrics['cpus_utilized']:.2f}\n")
                
                if 'parallelism' in metrics:
                    f.write(f"Parallelism: {metrics['parallelism']:.2f}x\n")
                
                if 'max_threads' in metrics:
                    f.write(f"Max threads: {metrics['max_threads']}\n")
                
                if 'wall_time' in metrics:
                    f.write(f"Duration: {metrics['wall_time']:.3f}s\n")
                
                output = result['stdout'] + result['stderr']
                if "DATA RACE" in output:
                    f.write("Detected data race\n")
                if "DEADLOCK" in output:
                    f.write("Detected deadlock\n")
                if "No data race" in output:
                    f.write("Okay\n")
                
                f.write("\n")
        
        print(f"Report was seved in: {report_path}")
        
        print("\nResults:")
        for result in results:
            status = "okay" if result.get('exit_code') == 0 else "bad"
            name = result.get('name', 'Unknown')

            duration = result.get('metrics', {}).get('wall_time', 0)

            cpu_info = ""
            if 'cpus_utilized' in result.get('metrics', {}):
                cpu_info = f", CPU: {result['metrics']['cpus_utilized']:.1f} cores"
            
            print(f"{status} {name:25} {duration:6.2f}s{cpu_info}")        
            
    def main(self):
        self.compile_cpp()
        
        results = self.run_all_demos()
        
        if results:
            self.generate_report(results)
        
def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--threads', '-t', type=int, default=4, help='Number of threads(default: 4)')
    
    args = parser.parse_args()
    
    controller = SimpleController(threads=args.threads)
    controller.main()
            
if __name__ == "__main__":
    main()