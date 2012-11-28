# psf -- Process Stack Faker (a.k.a. Fucker)
Coded by Stas; (C)opyLeft by SysD Destructive Labs, 1997-2003

Tested on: FreeBSD 4.3, Linux 2.4, NetBSD 1.5, Solaris 2.7

Compile with:

    # gcc -O2 -o psf psf.c
    # strip psf

Did you ever need to **hide** what are you doing on somewhat like public
server? Like Quake server or maybe John The Ripper? 'Cos when your admin
run `ps auwx` or `top` and sees process like that, it's probable you'll
loose your shell on that server. So, what to do? Rootkit is a good solution
but you need root privilegies to install it and it's a bit overkill for
running an inoffensive `eggdrop` bot (belive me, I saw user installing rootkit
just to hide `eggdrop`!). Well, this little proggie does a job for you. It
**will not** erase some entry you wish to hide from process stack. It just
changes a commandline for `ps  entry ;)

This principle is widely used in many security-related programs. Nmap was
the first I saw. How does this technique works? Take a look at execv(3)
system call:

    int execv( const char *path, char *const argv[]);

*path* is a path to executable file. And *argv* array is... Well, it's
just the same *argv* from:

    int main(int argc, char *argv[])

where *argv[0]* is a commandline and *argv[1]* and higher are paramenters.
Normally *argv[0]* receives the same value as *path* from execv(3). But you
also can use other values! For example, when you run Nmap, it can execv(3)
itself with commandline changed to *pine*. OK, commandline is gone. But what
to do with paramenters? Nmap uses environment to send paramenters user passed
to *spoofed* process and ignores other parameters. If you wish to spoof
`nmap -sS -vv -O -P0 -o lhost.log localhost` as `pine -i`, Nmap "remembers"
it's specific switches and re-execs itself as *pine* with parameter *-i*.
Fine! But John The Ripper, Quake server & eggdrop can't fake parameters in
this way!!! What's the other way? Sorry, it's *very* dumb and *very* ugly...
What happens if you change commandline to something like:

    'pine -i                                                            '

(Ya, `pine -i` plus many space characters 0x20)? Hahah, `ps`, `top` & many
other monitors just shift away **real** parameters! So, you don't hide them,
just shift away from screen. Such an "algorithm" doesn't needs neither rootkits,
neither special privilegies! Any user can do that at any time!!! **That's** what `psf`
does. Try this:

    # psf -s "pine -i" sleep 30 &
    [1] 440
    # ps auwx
    ...
    stas        84  0.0  0.6  2012 1232 pts/0    S    19:12   0:00 bash -rcfile .bashrc
    stas       440  0.0  0.1  1204  376 tty2     S    20:09   0:00 pine -i

    stas       450  0.0  0.4  2544  816 tty2     R    20:12   0:00 ps auwx
    ...

Hahahaah, that's what we need! Please note that commandline change isn't
immediate, just wait a little before it completes. But... Did you noticed
a white line between processes 440 & 450? Uhm, that's our "shift buffer".
Pray for your admin don't notice that! Anyway, they are many more problems
with parameter shifting. `top` program, for example, shows "command names"
instead of "command lines" by default. You see a file name instead of
*argv[0]* value. `psf` tries to fix that creating symlink with name of
faked commandline to real program (on previous example, it creates symlink
`/tmp/.psf-xxxx/pine => /usr/bin/sleep`). Note that it doesn't works on BSD
systems (BSD kernel (?) follows symlink and shows real filename anyway).
The ways to discover faked processes I know are:

 * kidding with top(1)
 * `ps auwx --cols 1024`
 * `cat /proc/[pidn]/cmdline` (Linux only)
 * whatever non-standart process stack monitors
 * looking open files with `lsof` program
 * if you use *-d* (daemonize) option, be careful!!! As any cool daemon should
   do, `psf` closes std(in,out,err). What your admin will think if he (she)
   sees `pine -i` with no parent and neither allocated TTY?!

Too many, don't you think? So, what's **THE BEST** way to hide processes?
Rootkit sounds well, but it's a bit complex to use, you know... So, IMHO,
you must get source of program you wish to hide and hardcode all parameters
inside executable... After that, rename it in whatever and let it go!
Of course you must program at least C/C++ to do such a trick. Now, if
you're glad with my quick & dirty solution called `psf`, happy faking!!!
