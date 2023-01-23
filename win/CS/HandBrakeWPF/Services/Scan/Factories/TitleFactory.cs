// --------------------------------------------------------------------------------------------------------------------
// <copyright file="TitleFactory.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the TitleFactory type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Scan.Factories
{
    using System;

    using HandBrake.App.Core.Model;
    using HandBrake.App.Core.Utilities;
    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.Interfaces.Model.Picture;
    using HandBrake.Interop.Interop.Json.Scan;

    using HandBrakeWPF.Services.Encode.Model.Models;
    using HandBrakeWPF.Services.Scan.Model;

    using Size = HandBrakeWPF.Model.Picture.Size;

    public class TitleFactory
    {
        public Title CreateTitle(SourceTitle title, int mainFeature)
        {
            string driveLabel = null;
            if ("VIDEO_TS".Equals(title.Name, StringComparison.CurrentCultureIgnoreCase))
            {
                foreach (DriveInformation info in DriveUtilities.GetDrives())
                {
                    if (title.Path.StartsWith(info.RootDirectory, StringComparison.CurrentCultureIgnoreCase))
                    {
                        driveLabel = info.VolumeLabel;
                        break;
                    }
                }
            }

            Title converted = new Title
            {
                TitleNumber = title.Index,
                Duration = new TimeSpan(0, title.Duration.Hours, title.Duration.Minutes, title.Duration.Seconds),
                Resolution = new Size(title.Geometry.Width, title.Geometry.Height),
                AngleCount = title.AngleCount,
                ParVal = new Size(title.Geometry.PAR.Num, title.Geometry.PAR.Den),
                AutoCropDimensions = new Cropping
                {
                    Top = title.Crop[0],
                    Bottom = title.Crop[1],
                    Left = title.Crop[2],
                    Right = title.Crop[3]
                },
                LooseCropDimensions = new Cropping()
                {
                    Top = title.LooseCrop[0],
                    Bottom = title.LooseCrop[1],
                    Left = title.LooseCrop[2],
                    Right = title.LooseCrop[3]
                },
                Fps = ((double)title.FrameRate.Num) / title.FrameRate.Den,
                SourceName = title.Path,
                DriveLabel = driveLabel,
                MainTitle = mainFeature == title.Index,
                Playlist = title.Type == 1 ? string.Format(" {0:d5}.MPLS", title.Playlist).Trim() : null,
                FramerateNumerator = title.FrameRate.Num,
                FramerateDenominator = title.FrameRate.Den,
                Type = title.Type
            };

            int currentTrack = 1;
            foreach (SourceChapter chapter in title.ChapterList)
            {
                string chapterName = !string.IsNullOrEmpty(chapter.Name) ? chapter.Name : string.Empty;
                converted.Chapters.Add(new Chapter(currentTrack, chapterName, new TimeSpan(chapter.Duration.Hours, chapter.Duration.Minutes, chapter.Duration.Seconds)));
                currentTrack++;
            }

            int currentAudioTrack = 1;
            foreach (SourceAudioTrack track in title.AudioList)
            {
                converted.AudioTracks.Add(new Audio(currentAudioTrack, track.Language, track.LanguageCode, track.Description, track.Codec, track.SampleRate, track.BitRate, track.ChannelLayout, track.Name));
                currentAudioTrack++;
            }

            int currentSubtitleTrack = 1;
            foreach (SourceSubtitleTrack track in title.SubtitleList)
            {
                SubtitleType convertedType = (SubtitleType)track.Source;
    
                bool canBurn = HandBrakeSubtitleHelpers.CheckCanBurnSubtitle(track.Source) > 0;
                bool canSetForcedOnly = HandBrakeSubtitleHelpers.CheckCanForceSubtitle(track.Source) > 0;

                converted.Subtitles.Add(new Subtitle(track.Source, currentSubtitleTrack, track.Language, track.LanguageCode, convertedType, canBurn, canSetForcedOnly, track.Name, false));
                currentSubtitleTrack++;
            }

            SourceMetadata metadata = title.MetaData;
            if (title.MetaData != null)
            {
                converted.Metadata = new Metadata(
                    metadata.AlbumArtist,
                    metadata.Album,
                    metadata.Artist,
                    metadata.Comment,
                    metadata.Composer,
                    metadata.Description,
                    metadata.Genre,
                    metadata.LongDescription,
                    metadata.Name,
                    metadata.ReleaseDate);
            }

            return converted;
        }
    }
}
