using System;
using System.Collections.Generic;
using System.Text;
using System.Collections;

namespace Handbrake.Functions
{
    public class Queue
    {
        ArrayList queue = new ArrayList();
        string lastQuery;

        public ArrayList getQueue()
        {
            return queue;
        }

        /// <summary>
        /// Get's the next CLI query for encoding
        /// </summary>
        /// <returns>String</returns>
        public string getNextItemForEncoding()
        {
            string query = queue[0].ToString();
            lastQuery = query;
            remove(0);
            return query;
        }

        /// <summary>
        /// Add's a new item to the queue
        /// </summary>
        /// <param name="query">String</param>
        public void add(string query)
        {
            queue.Add(query);
        }

        /// <summary>
        /// Removes an item from the queue.
        /// </summary>
        /// <param name="index">Index</param>
        /// <returns>Bolean true if successful</returns>
        public Boolean remove(int index)
        {
            try
            {
                queue.RemoveAt(index);
                return true;
            }
            catch (Exception)
            {
                return false;
            }
        }

        /// <summary>
        /// Returns how many items are in the queue
        /// </summary>
        /// <returns>Int</returns>
        public int count()
        {
            return queue.Count;
        }

        /// <summary>
        /// Get's the last query to be selected for encoding by getNextItemForEncoding()
        /// </summary>
        /// <returns>String</returns>
        public string getLastQuery()
        {
            return lastQuery;
        }

        /// <summary>
        /// Move an item with an index x, up in the queue
        /// </summary>
        /// <param name="index">Int</param>
        public void moveUp(int index)
        {
            if (index != 0)
            {
                string item = queue[index].ToString();

                queue.Insert((index - 1), item);
                queue.RemoveAt((index + 1));
            }
        }

        /// <summary>
        /// Move an item with an index x, down in the queue
        /// </summary>
        /// <param name="index">Int</param>
        public void moveDown(int index)
        {
            if (index != queue.Count - 1)
            {
                string item = queue[index].ToString();

                queue.Insert((index + 2), item);
                queue.RemoveAt((index));
            }
        }

    }
}
