# Usage: Set environment variables, then run ./translate_windows.sh
# Using your own HandBrake Fork, example:
#     export HB_GIT_REPO=https://github.com/<your-username>/HandBrake.git
#     export HB_BRANCH_NAME=translation_update
# This works with the "Download All" zip packages transifex creates.
#     The download all, Goto the "Resources" age, 
#     Click on the resource to delve into it
#     The three dots button top right will have a Download all option.



if [[ -z ${HB_GIT_REPO} ]]; then
    echo "HB_GIT_REPO should be set to your HandBrake Repo Fork URL, including .git";
    exit 1;
fi

if [[ -z ${HB_BRANCH_NAME} ]]; then
    echo "HB_BRANCH_NAME should be set to a new unique branch name for updating translations";
    exit 1;
fi

echo ""
echo "Using: "
echo "Git Repo: $HB_GIT_REPO"
echo "Git Branch: $HB_BRANCH_NAME"
echo ""

# Cleanup Past Runs
echo ""
echo "- Tidyup any previous run"
rm -rf HandBrake
rm *.resx

# Create a bit branch for the pull request
echo ""
echo "- Download git fork and create a translation branch."
echo $HB_GIT_REPO
git clone $HB_GIT_REPO
git branch translation_update
git checkout translation_update

# Unpack
echo ""
echo "- Unpacking the transifex files"
unzip handbrakeproject_winui_resourcesresx.zip
unzip handbrakeproject_winui_resourcestooltipsresx.zip

# Rename the files
echo ""
echo "- Renaming the files"
for f in *; do
        case $f in 
               resourcestooltipsresx_*.resx)
                        [[ $f =~ resourcestooltipsresx_(.*) ]]
                        suffix=${BASH_REMATCH[1]}
                        y="ResourcesTooltips.$suffix"
                        echo "Renaming $f to $y"
                        mv $f $y
                        ;;
               resourcesresx_*.resx)
                        [[ $f =~ resourcesresx_(.*) ]]
                        suffix=${BASH_REMATCH[1]}
                        y="Resources.$suffix"
                        echo "Renaming $f to $y"
                        mv $f $y
                        ;;
        esac
done

# Change the line endings to Windows
echo ""
echo "- Changing Line Endings"
unix2dos *.resx

# Copy the files to the correct directory
echo ""
echo "- Copying files to the correct directory"
cp *.resx HandBrake/win/CS/HandBrakeWPF/Properties

# Commit the change
echo ""
echo "- Creating a Git commit"
cd HandBrake/win/CS/HandBrakeWPF/Properties
git add *.resx
cd ../../../../
git commit -m "Updating Windows Translations"

echo ""
echo "Done: git push then create a pull request on GitHub."
