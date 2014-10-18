using System;
using System.Diagnostics;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Windows.Forms;

namespace XRay.SdkControls
{
    /// <summary>
    ///     Represents a SORTED collection of IntegerUpDownAcceleration objects in the NumericUpDown Control.
    ///     The elements in the collection are sorted by the IntegerUpDownAcceleration.Seconds property.
    /// </summary>
    [ListBindable(false)]
    public class IntegerUpDownAccelerationCollection :
        MarshalByRefObject,
        ICollection<IntegerUpDownAcceleration>,
        IEnumerable<IntegerUpDownAcceleration>
    {
        private List<IntegerUpDownAcceleration> items;

        /// <summary>
        ///     Class constructor.
        /// </summary>
        public IntegerUpDownAccelerationCollection()
        {
            items = new List<IntegerUpDownAcceleration>();
        }

        /// <summary>
        ///     Gets (ReadOnly) the element at the specified index. In C#, this property is the indexer for
        ///     the IList class.
        /// </summary>
        /// <param name="index"></param>
        /// <returns></returns>
        public IntegerUpDownAcceleration this[int index]
        {
            get
            {
                return items[index];
            }
        }

        // ICollection<IntegerUpDownAcceleration> implementation. 

        /// <summary>
        ///     Adds an item (IntegerUpDownAcceleration object) to the ICollection.
        ///     The item is added preserving the collection sorted.
        /// </summary>
        /// <param name="acceleration"></param>
        public void Add(IntegerUpDownAcceleration acceleration)
        {
            if (acceleration == null)
            {
                throw new ArgumentNullException("acceleration");
            }

            // Keep the array sorted, insert in the right spot.
            var index = 0;

            while (index < items.Count)
            {
                if (acceleration.Seconds < items[index].Seconds)
                {
                    break;
                }
                ++index;
            }
            items.Insert(index, acceleration);
        }

        /// <summary>
        ///     Removes all items from the ICollection.
        /// </summary>
        public void Clear()
        {
            items.Clear();
        }

        /// <summary>
        ///     Determines whether the IList contains a specific value.
        /// </summary>
        /// <param name="acceleration"></param>
        /// <returns></returns>
        public bool Contains(IntegerUpDownAcceleration acceleration)
        {
            return items.Contains(acceleration);
        }

        /// <summary>
        ///     Copies the elements of the ICollection to an Array, starting at a particular Array index.
        /// </summary>
        /// <param name="array"></param>
        /// <param name="index"></param>
        public void CopyTo(IntegerUpDownAcceleration[] array, int index)
        {
            items.CopyTo(array, index);
        }

        /// <summary>
        ///     Gets the number of elements contained in the ICollection.
        /// </summary>
        public int Count
        {
            get
            {
                return items.Count;
            }
        }

        /// <summary>
        ///     Gets a value indicating whether the ICollection is read-only.
        ///     This collection property returns false always.
        /// </summary>
        public bool IsReadOnly
        {
            get
            {
                return false;
            }
        }

        /// <summary>
        ///     Removes the specified item from the ICollection.
        /// </summary>
        /// <param name="acceleration"></param>
        /// <returns></returns>
        public bool Remove(IntegerUpDownAcceleration acceleration)
        {
            return items.Remove(acceleration);
        }

        // IEnumerable<IntegerUpDownAcceleration> implementation.


        /// <summary>
        ///     Returns an enumerator that can iterate through the collection.
        /// </summary>
        /// <returns></returns>
        IEnumerator<IntegerUpDownAcceleration> IEnumerable<IntegerUpDownAcceleration>.GetEnumerator()
        {
            return items.GetEnumerator();
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return ((IEnumerable)items).GetEnumerator();
        }

        // IntegerUpDownAccelerationCollection methods.

        /// <summary>
        ///     Adds the elements of specified array to the collection, keeping the collection sorted.
        /// </summary>
        /// <param name="accelerations"></param>
        public void AddRange(params IntegerUpDownAcceleration[] accelerations)
        {
            if (accelerations == null)
            {
                throw new ArgumentNullException("accelerations");
            }

            // Accept the range only if ALL elements in the array are not null.
            foreach (var acceleration in accelerations)
            {
                if (acceleration == null)
                {
                    throw new ArgumentNullException("At least oe entry is null");
                }
            }

            // The expected array size is typically small (5 items?), so we don't need to try to be smarter about the
            // way we add the elements to the collection, just call Add.
            foreach (var acceleration in accelerations)
            {
                Add(acceleration);
            }
        }
    }
}
