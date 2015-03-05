// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ScanFactory.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The scan factory.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.Factories
{
    using System;
    using System.Collections.Generic;

    using HandBrake.ApplicationServices.Interop.Json.Scan;
    using HandBrake.ApplicationServices.Interop.Model;
    using HandBrake.ApplicationServices.Interop.Model.Scan;

    /// <summary>
    /// This factory takes the JSON objects deserialized from libhb for the scan information
    /// and converts them into the internal Title objects.
    /// </summary>
    internal class ScanFactory
    {
        /// <summary>
        /// The create title set.
        /// </summary>
        /// <param name="scan">
        /// The scan.
        /// </param>
        /// <returns>
        /// The <see cref="IEnumerable"/>.
        /// </returns>
        internal static IEnumerable<Title> CreateTitleSet(JsonScanObject scan)
        {
            List<Title> titles = new List<Title>();

            if (scan != null)
            {
                foreach (TitleList item in scan.TitleList)
                {
                    Title title = CreateTitle(item);

                    if (title.TitleNumber == scan.MainFeature)
                    {
                        title.IsMainFeature = true;
                    }

                    titles.Add(title);
                }
            }

            return titles;
        }

        /// <summary>
        /// The create title.
        /// </summary>
        /// <param name="title">
        /// The title.
        /// </param>
        /// <returns>
        /// The <see cref="Title"/>.
        /// </returns>
        private static Title CreateTitle(TitleList title)
        {
            Title newTitle = new Title
            {
                TitleNumber = title.Index,
                Playlist = title.Playlist,
                Resolution = new Size(title.Geometry.Width, title.Geometry.Height),
                ParVal = new Size(title.Geometry.PAR.Num, title.Geometry.PAR.Den), 
                Duration = new TimeSpan(title.Duration.Hours, title.Duration.Minutes, title.Duration.Seconds),
                AutoCropDimensions = new Cropping
                {
                    Top = title.Crop[0],
                    Bottom = title.Crop[1],
                    Left = title.Crop[2],
                    Right = title.Crop[3]
                },
                AngleCount = title.AngleCount,
                VideoCodecName = title.VideoCodec,
                Framerate = ((double)title.FrameRate.Num) / title.FrameRate.Den,
                FramerateNumerator = title.FrameRate.Num,
                FramerateDenominator = title.FrameRate.Den,
                Path = title.Path
            };

            switch (title.Type)
            {
                case 2:
                    newTitle.InputType = InputType.Stream;
                    break;
                case 0:
                    newTitle.InputType = InputType.Dvd;
                    break;
                case 1:
                    newTitle.InputType = InputType.Bluray;
                    break;
                      case 3:
                    newTitle.InputType = InputType.FFStream;
                    break;
            }

            foreach (Subtitle subtitleTrack in CreateSubtitleTracks(title.SubtitleList))
            {
                newTitle.Subtitles.Add(subtitleTrack);
            }

            foreach (AudioTrack audioTrack in CreateAudioTracks(title.AudioList))
            {
                newTitle.AudioTracks.Add(audioTrack);
            }

            foreach (Chapter chapter in CreateChapters(title.ChapterList))
            {
                newTitle.Chapters.Add(chapter);
            }
  
            return newTitle;
        }

        /// <summary>
        /// The create subtitle tracks.
        /// </summary>
        /// <param name="subtitles">
        /// The subtitles.
        /// </param>
        /// <returns>
        /// The <see cref="IEnumerable"/>.
        /// </returns>
        private static IEnumerable<Subtitle> CreateSubtitleTracks(IEnumerable<SubtitleList> subtitles)
        {
            List<Subtitle> subtiles = new List<Subtitle>();

            int currentSubtitleTrack = 1;
            foreach (SubtitleList subtitle in subtitles)
            {
                Subtitle newSubtitle = new Subtitle
                {
                    TrackNumber = currentSubtitleTrack,
                    Language = subtitle.Language,
                    LanguageCode = subtitle.LanguageCode,
                    SubtitleSourceInt = subtitle.Source,
                    SubtitleSource = (SubtitleSource)subtitle.Source  // TODO Check correct
                };

                subtiles.Add(newSubtitle);

                currentSubtitleTrack++;
            }

            return subtiles;
        }

        /// <summary>
        /// The create audio tracks.
        /// </summary>
        /// <param name="audioTracks">
        /// The audio tracks.
        /// </param>
        /// <returns>
        /// The <see cref="IEnumerable"/>.
        /// </returns>
        private static IEnumerable<AudioTrack> CreateAudioTracks(IEnumerable<AudioList> audioTracks)
        {
            List<AudioTrack> tracks = new List<AudioTrack>();

            int currentAudioTrack = 1;
            foreach (AudioList track in audioTracks)
            {
                AudioTrack newAudio = new AudioTrack
                {
                    TrackNumber = currentAudioTrack,
                    CodecId = Convert.ToUInt32(track.Codec),
                    Language = track.Language,
                    LanguageCode = track.LanguageCode,
                    Description = track.Description,
                    ChannelLayout = (ulong)track.ChannelLayout,
                    SampleRate = track.SampleRate,
                    Bitrate = track.BitRate
                };

                tracks.Add(newAudio);

                currentAudioTrack++;
            }
            return tracks;
        }

        /// <summary>
        /// The create chapters.
        /// </summary>
        /// <param name="chapters">
        /// The chapters.
        /// </param>
        /// <returns>
        /// The <see cref="IEnumerable"/>.
        /// </returns>
        private static IEnumerable<Chapter> CreateChapters(IEnumerable<ChapterList> chapters)
        {
            List<Chapter> tracks = new List<Chapter>();

            int currentTrack = 1;
            foreach (ChapterList chapter in chapters)
            {
                Chapter newChapter = new Chapter
                {
                    Name = chapter.Name,
                    ChapterNumber = currentTrack,
                    Duration = new TimeSpan(chapter.Duration.Hours, chapter.Duration.Minutes, chapter.Duration.Seconds)
                };

                tracks.Add(newChapter);
                currentTrack++;
            }

            return tracks;
        }
    }
}
