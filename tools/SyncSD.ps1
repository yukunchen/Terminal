$OC_User = $env:OC_User
$OC_Pass = $env:OC_Pass

# Fetch last git item and extract work item IDs
$lastGit = git log -1
$checkinIDs = Select-String "#[\d]*" -input $lastGit -AllMatches | % matches | % Value | % Substring(1)

# Go into VSO and get the bug/task data associated
$checkinQueryIDString = [String]::Join(",", $checkinIDs)
$Uri = "https://microsoft.visualstudio.com/DefaultCollection/_apis/wit/workitems?ids=" + $checkinQueryIDString + "&api-version=1.0"
$base64authinfo = [Convert]::ToBase64String([Text.Encoding]::ASCII.GetBytes(("{0}:{1}" -f $OC_User, $OC_Pass)))
$responseFromGet = Invoke-RestMethod -Method Get -ContentType application/json -Uri $Uri -Headers @{Authorization=("Basic {0}" -f $base64authinfo)} 

# Dig through JSON and pull out ID and title
$changedItems = $responseFromGet.value | select id, @{Name="title"; Expression={$_.fields | select System.Title | % System.Title}}

# Prepare changelog record
$date = Get-Date
$changelogHead = "`r`n{0}/{1}/{2} - <auto merge>" -f $date.Month, $date.Day, $date.Year
$changelogChildren = foreach ($a in $changedItems) { "`t{0} - {1}" -f $a.id, $a.title}

# Write changelog record
attrib -r changelog.txt
Add-Content changelog.txt $changelogHead
Add-Content changelog.txt $changelogChildren

git add changelog.txt

# Now update sdcheckin.txt
$sdcheckin = Get-Content sdcheckin.txt

$newids = [String]::Join(" ", $checkinIDs)
$newidtag = "MSFT: {0} -" -f $newids

# replace existing ID string with new
$sdnew = $sdcheckin -replace "MSFT:[ \d]*-",$newidtag

# Write sdcheckin record
attrib -r sdcheckin.txt
Set-Content sdcheckin.txt $sdnew

git add sdcheckin.txt

git commit -m "Auto-build update of sdcheckin and changelog"
git push

