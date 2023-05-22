// --------------------------------------------------------------------------------------------------------------------
// <copyright file="OrderedListDictionary.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A crude dictionary that also has a list that's ordered by inserstion value.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Helpers
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Linq;

    public class OrderedListDictionary<T, TK> : IDictionary<T, TK>
    {
        Dictionary<T, TK> baseData = new Dictionary<T, TK>();
        List<TK> baseDataList = new List<TK>();

        public int Count => this.baseData.Count;

        public bool IsReadOnly => false;

        public ICollection<T> Keys => this.baseData.Keys;

        public ICollection<TK> Values => this.baseData.Values;

        public List<TK> FlatList => this.baseDataList;

        public IEnumerator<KeyValuePair<T, TK>> GetEnumerator()
        {
            return this.baseData.GetEnumerator();
        }

        public void Add(KeyValuePair<T, TK> item)
        {
            if (item.Value == null)
            {
                return;
            }

            this.baseData.Add(item.Key, item.Value);
            this.baseDataList.Add(item.Value);
        }

        public void Clear()
        {
            this.baseData.Clear();
            this.baseDataList.Clear();
        }

        public bool Contains(KeyValuePair<T, TK> item)
        {
            return this.baseData.Contains(item);
        }

        public void CopyTo(KeyValuePair<T, TK>[] array, int arrayIndex)
        {
            throw new NotImplementedException();
        }

        public bool Remove(KeyValuePair<T, TK> item)
        {
            if (item.Value == null)
            {
                return false;
            }

            this.baseDataList.Remove(item.Value);
            return this.baseData.Remove(item.Key);
        }


        public void Add(T key, TK value)
        {
            this.baseData.Add(key, value);
            this.baseDataList.Add(value);
        }

        public bool ContainsKey(T key)
        {
            return this.baseData.ContainsKey(key);
        }

        public bool Remove(T key)
        {
            if (this.baseData.ContainsKey(key))
            {
                TK item = this.baseData[key];
                this.baseDataList.Remove(item);
                return this.baseData.Remove(key);
            }

            return false;
        }

        public bool TryGetValue(T key, out TK value)
        {
            return this.baseData.TryGetValue(key, out value);
        }

        public TK this[T key]
        {
            get => this.baseData[key];
            set => this.baseData[key] = value;
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return this.baseData.GetEnumerator();
        }
    }
}
