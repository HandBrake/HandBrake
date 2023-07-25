# Usage: Set environment variables, then run ./translate_mac.sh
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
rm *.xlf

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
unzip handbrakeproject_mac-ui_enxliff*.zip 

# Run xcode to manage the translations
echo ""
echo "- Process Translation Files"
for f in *; do
        case $f in 
               enxliff_*.xlf)
                        [[ $f =~ enxliff_(.*) ]]
                        suffix=${BASH_REMATCH[1]}
                        y="$suffix"
                  						
						echo "Processing $f ... "
                        xcodebuild -importLocalizations -project /HandBrake/macosx/HandBrake.xcodeproj -localizationPath $f 
						mv /HandBrake/macosx/build/release/external/macosx/$y.lproj/Localizable.strings /HandBrake/macosx/HandBrakeKit/$y.lproj
						;;
        esac
done

# Commit the change
echo ""
echo "- Creating a Git commit"
cd HandBrake
git add --all
git commit -m "Updating Mac UI Translations"

echo ""
echo "Done: git push then create a pull request on GitHub."
