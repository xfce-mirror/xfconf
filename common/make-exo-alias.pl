#!/usr/bin/env perl -w
#
# Copyright (c) 2004 The GLib Development Team.
# Copyright (c) 2005 Benedikt Meurer <benny@xfce.org>.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 of the License ONLY.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
#

my $option_def = 0;

if (($#ARGV >= 0) && ($ARGV[0] eq "-def"))
  {
    shift;
    $option_def = 1;
  }

print <<EOF;
/* Generated by make-exo-alias.pl. Do not edit this file. */

#ifdef HAVE_GNUC_VISIBILITY

#include <glib.h>

EOF

my $in_comment = 0;
my $in_skipped_section = 0;

while (<>)
  {
    # ignore empty lines
    next if /^\s*$/;

    # skip comments
    if ($_ =~ /^\s*\/\*/)
      {
        $in_comment = 1;
      }
    
    if ($in_comment)
      {
        if ($_ =~  /\*\/\s$/)
          {
            $in_comment = 0;
          }
        next;
      }

    # handle ifdefs
    if ($_ =~ /^\#endif/)
      {
        if (!$in_skipped_section)
          {
            print $_;
          }

        $in_skipped_section = 0;
        next;
      }

    if ($_ =~ /^\#ifdef\s+(INCLUDE_VARIABLES|INCLUDE_INTERNAL_SYMBOLS|ALL_FILES)/)
      {
        $in_skipped_section = 1;
      }

    if ($in_skipped_section)
      {
        next;
      }

    if ($_ =~ /^\#ifn?def\s+G/)
      {
        print $_;
        next;
      }

    if ($_ =~ /^\#if.*IN_SOURCE\((.*)\)/)
      {
        if ($option_def)
          {
            print "#ifdef $1\n";
          }
        else
          {
            print "#if 1\n";
          }
        next;
      }

    if ($_ =~ /^\#if.*IN_HEADER\((.*)\)/)
      {
        if ($option_def)
          {
            print "#if 1\n";
          }
        else
          {
            print "#ifdef $1\n";
          }
        next;
      }

    chop;
    my $line = $_;
    my @words;
    my $attributes = "";

    @words = split (/ /, $line);
    my $symbol = shift (@words);
    chomp ($symbol);
    my $alias = "IA__".$symbol;
    
    # Drop any Win32 specific .def file syntax,  but keep attributes
    foreach $word (@words)
      {
        $attributes = "$attributes $word" unless $word eq "PRIVATE";
      }
    
    if (!$option_def)
      {
        print <<EOF
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
extern __typeof ($symbol) $alias __attribute((visibility("hidden")))$attributes;
G_GNUC_END_IGNORE_DEPRECATIONS
\#define $symbol $alias

EOF
      }
    else
      {
        print <<EOF
\#undef $symbol 
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
extern __typeof ($symbol) $symbol __attribute((alias("$alias"), visibility("default")));
G_GNUC_END_IGNORE_DEPRECATIONS

EOF
      }
  }

print <<EOF;

#endif /* HAVE_GNUC_VISIBILITY */
EOF


