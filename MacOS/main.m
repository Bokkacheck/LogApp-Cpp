#import <Foundation/Foundation.h>
#import <AppKit/NSWorkspace.h>


int main (int argc, const char * argv[])
{
    int work = 1;
    int camera = 0;

    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSString *userName = NSUserName();
    NSString *path = NSHomeDirectory();
    
    pathApp = [path stringByAppendingString:@"/log-app"];
    [[NSFileManager defaultManager] createFileAtPath:pathApp contents:nil attributes:nil];
    NSString *str = [@"LOGIN:-:" stringByAppendingString:userName];
    [str writeToFile:path atomically:YES encoding:NSUTF8StringEncoding error:nil];
    
    pathCamera = [path stringByAppendingString:@"/log-camera"];
    [[NSFileManager defaultManager] createFileAtPath:pathCamera contents:nil attributes:nil];

    while(work == 1){
        NSRunningApplication *frontmostApplication = [[NSWorkspace sharedWorkspace] frontmostApplication];
        NSString *focusedApp = [frontmostApplication localizedName];
        if(focusedApp!=lastApp){
            str = [focusedApp stringByAppendingString:@"\n"];
            [str writeToFile:path atomically:YES encoding:NSUTF8StringEncoding error:nil];
            lastApp = focusedApp;
        }

        int pid = [[NSProcessInfo processInfo] processIdentifier];
        NSPipe *pipe = [NSPipe pipe];
        NSFileHandle *file = pipe.fileHandleForReading;
        NSTask *task = [[NSTask alloc] init];
        task.launchPath = @"/usr/bin/lsof";
        task.arguments = @[ @"|",@"grep",@"AppleCamera"];
        task.standardOutput = pipe;
        [task launch];
        NSData *data = [file readDataToEndOfFile];
        [file closeFile];
        NSString *result = [[NSString alloc] initWithData: data encoding: NSUTF8StringEncoding];
        if(result==""){
            if(camera == 1){
                [@"Camera DISABLED" writeToFile:pathCamera atomically:YES encoding:NSUTF8StringEncoding error:nil];
            }
            camera = 0;
        }else{
            if(camera == 0){
                [@"Camera ACTIVATED" writeToFile:pathCamera atomically:YES encoding:NSUTF8StringEncoding error:nil];
            }
            camera = 1;
        }
        
        sleep(0.1);
    }
    return 0;
}



