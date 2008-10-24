#
# $Header: /cvsroot/gtk2-perl/gtk2-perl-xs/Glade/Makefile.PL,v 1.21 2008/09/07 20:05:50 kaffeetisch Exp $
#

use strict;
use 5.008;
use Cwd;
use File::Spec;
use ExtUtils::MakeMaker;

# minimum required version of dependancies we need to build
our %build_reqs = (
	'perl-ExtUtils-Depends'   => '0.300',
	'perl-ExtUtils-PkgConfig' => '1.000',
	'perl-Glib'               => '1.020', # for Glib::MakeHelper
	'Xfconf'                  => '4.5.91',
);

our %pre_reqs = (
	Glib::                => $build_reqs{'perl-Glib'},
	ExtUtils::Depends::   => $build_reqs{'perl-ExtUtils-Depends'},
	ExtUtils::PkgConfig:: => $build_reqs{'perl-ExtUtils-PkgConfig'},
);

# Writing a fake Makefile ensures that CPAN will pick up the correct
# dependencies and install them.
unless(eval "use ExtUtils::Depends '$build_reqs{'perl-ExtUtils-Depends'}';"
          . "use ExtUtils::PkgConfig '$build_reqs{'perl-ExtUtils-PkgConfig'}';"
          . "use Glib::MakeHelper;"
          . "use Glib::CodeGen;"
          # just seeing if Glib is available isn't enough, make sure
          # it's recent enough, too 
          . "use Glib '$build_reqs{'perl-Glib'}';"
          . "1")
{
    warn "$@\n";
    WriteMakefile(
        PREREQ_FATAL => 1,
        PREREQ_PM    => \%pre_reqs,
    );
    exit 1; # not reached
}

# If the package can't be found, warn and exit with status 0 to indicate to
# CPAN testers that their system is not supported.
our %pkgcfg;
unless(eval { %pkgcfg = ExtUtils::PkgConfig->find ('libxfconf-0 >= '.$build_reqs{Xfconf});
	           1; })
{
    warn $@;
    exit 0;
}

require Glib::CodeGen;

mkdir('build', 0777);

my @xs_files = <xs/*.xs>;

Glib::CodeGen->parse_maps('xfconfperl',
                          input => 'xfconfperl.maps');
Glib::CodeGen->write_boot(xs_files =>\@xs_files,
                          ignore => '(^[^:]+$)|(^Xfce4::Xfconf$)');

my $cwd = cwd();
my $xfconf = ExtUtils::Depends->new('Xfconf', 'Glib');
$xfconf->set_inc($pkgcfg{cflags} . " -I. -Ibuild -I$cwd/..");
# this next bit is probably REALLY not portable
$xfconf->set_libs($pkgcfg{libs}. "-L$cwd/../common/.libs -lxfconf-gvaluefuncs");
$xfconf->add_pm('Xfconf.pm' => '$(INST_LIBDIR)/Xfconf.pm');
$xfconf->add_xs(@xs_files);
$xfconf->add_typemaps(map {File::Spec->catfile($cwd,$_)} 'xfconfperl.typemap');

$xfconf->install(qw(xfconfperl.h build/xfconfperl-autogen.h));
$xfconf->save_config('build/IFiles.pm');

WriteMakefile(
    NAME          => 'Xfce4::Xfconf',
    VERSION_FROM  => 'Xfconf.pm',
    ABSTRACT_FROM => 'Xfconf.pm',
    PREREQ_PM     => \%pre_reqs,
    XSPROTOARG    => '-noprototypes',
    $xfconf->get_makefile_vars,
);


sub MY::postamble
{
    return Glib::MakeHelper->postamble_clean ()
           . Glib::MakeHelper->postamble_rpms(
                'XFCONF' => $build_reqs{'Xfconf'},
                'PERL_EXTUTILS_DEPENDS' => $build_reqs{'perl-ExtUtils-Depends'},
                'PERL_EXTUTILS_PKGCONFIG' => $build_reqs{'perl-ExtUtils-PkgConfig'},
                'PERL_GLIB' => $build_reqs{'perl-Glib'},
	         );
}
