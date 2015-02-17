#! /usr/bin/perl

# this script opens a ghettoVCB logfile and checks if it 
# is from a valid time period.
# Usage: check_date.pl <logfilename> <days>
# where logfilename is the name of the logfile :)
# and days determines how old the backup is allowed to be in days 
#
# version 0.3, 8.10.2014, Clemens Feuerstein

use DateTime::Format::Strptime;
use Date::Calc qw/:all/;

# state definitions for nagios
$UNKNOWN = 3;
$CRITICAL = 2;
$WARNING = 1;
$OK = 0;


sub get_record
{
  # 2013-11-24
  my $strp = DateTime::Format::Strptime->new(
      pattern   => '%F',
      on_error  => 'undef',
  );
  
  my $dt;
  # the filename should be passed in as a parameter
  my $filename = shift;
  open FILE, $filename or die "Could not read from $filename, program halting.";
  # read the record, and chomp off the newline
  my $counter;
  my $record;
  do
  {
#    chomp($record = <FILE>);
    $record = <FILE>;
    $offset = index($record, ' ');
    $date = substr($record, 0, $offset);
    $length = length($record);
#    print "record: $record\nlength is: $length\ndate: $date\noffset: $offset\n\n";
    $counter += 1;
  } while ($record && $date !~ /(\d{4})-(\d\d)-(\d\d)/);
  # check if it is a valid date!
  if($counter == 1)
  {
    print "logfile only contains one line, maybe backup is corrupt\n";
    exit $CRITICAL;
  }
  if($date =~ /(\d{4})-(\d\d)-(\d\d)/)
  {
    $dt = $strp->parse_datetime($date);
  } else
  {
    print "no valid date found\n";
    exit $CRITICAL;
  }
# print "date object: $dt\n";
  close FILE;
  return $dt;
}

# quit unless we have the correct number of command-line args
$num_args = $#ARGV + 1;
if ($num_args < 2 && $num_args > 3) {
  print "\nUsage: check_date.pl <filename of log> <days> <performance_data>\n";
  exit $UNKNOWN;
}

my $logfile_name     = $ARGV[0];
my $max_days         = $ARGV[1];
my $performance_data = $ENV{'CHECK_LOGFILES_SERVICEPERFDATA'};
my $state            = $ENV{'CHECK_LOGFILES_SERVICESTATEID'};

# get date of logfile + current date
my $dt = &get_record($logfile_name);
my $current = DateTime->now;

#transform dates to days and calc difference
my $current_days = Date_to_Days($current->year, $current->month, $current->day);
my $dt_days = Date_to_Days($dt->year, $dt->month, $dt->day);
my $difference = $current_days - $dt_days;

#check if difference is ok!
if($difference > $max_days)
{
  printf "CRITICAL - Last backup on %d.%d.%d., should not be older than %d days | %s\n" , $dt->day, $dt->month, $dt->year, $max_days, $performance_data;
  exit $CRITICAL;
} 

printf "%s", $ENV{'CHECK_LOGFILES_SERVICESTATE'};
if($state == 0)
{
  printf " - Backup successfully executed on %d.%d.%d. | %s\n", $dt->day, $dt->month, $dt->year, $performance_data;
} else
{
  printf " - Backup executed on %d.%d.%d. | %s\n", $dt->day, $dt->month, $dt->year, $performance_data;
}


exit $state;
