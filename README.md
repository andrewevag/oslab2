# oslab2
- ### Παναγιώτα-Νικολέττα Μπάρμπα - el18604
- ### Ευαγγελάτος Ανδρέας - el18069


## Parameter
Το lunix εχει parameter :
 ``` C
  int lunix_sensor_cnt
 ```
το οποίο ρυθμίζουμε κατά το <i>insmod</i>:
```console
> insmod lunix.ko lunix_sensor_cnt=<number>
```


Έχουμε k αισθητήρες οι οποίοι έχουν
έως 8 διαφορετικές μετρήσεις.
Αυτές εδώ είναι batt (battery) 0, 
temp (temperature) 1, brightness (>>) 2,
και κάθε μέτρηση πρέπει να εμφανίζεται ως
διαφορετική συσκευή στο σύστημα.

Αυτές έχουν Major Number 60 και
Minor Numbers (sensor_id * 8 + measurement_num).

---
## init()
Aπό τον <i>lunix_sensor_cnt</i> που καθορίζει το <i>module.c</i> έχουμε τον αριθμό των συσκευών που αφορούν μετρήσεις από <i>lunix_sensor_cnt * 8</i> (αν και υποστηρίζουμε μόνο 3 ανα sensor).
Οπότε με το <i>register_chrdev_region()</i> ορίζουμε πως θα χειριζόμαστε κλήσεις συστήματος για όσες συσκευές έχουν <b>Major Number 60 και
Minor Numbers (sensor_id * 8 + measurement_num)</b>.
<br><br>
Ο πυρήνας θέλει να ορίσουμε και μία δομή <i>cdev</i> με την οποία 
συσχετίζει τις κλήσεις συστήματος με τις δικές μας ορισμένες συναρτήσεις και για ποιες συσκευες αυτές αναφέρονται.


---
## open()
``` C
    int open (struct inode *, struct file *)
```

Χρησιμοποιούμε
``` C
    unsigned int iminor(struct inode *inode);
```
ώστε να μπορούμε να ανακτήσουμε τον minor number του αρχείου, ώστε το ανοιχτό
αρχείο που θα προκύψει να αφορά συγκεκριμένο αισθητήρα και μέτρηση του
δικτύου.
O πυρήνας αρχικοποιεί κάποια δομή <b>file</b> και την περνάει 
στην open. Αυτή έχει τα εξής πεδία που μας ενδιαφέρουν
``` C
    struct file{
        mode_t f_mode; //Πρόσβαση
        loff_t f_pos;  //Δείκτης εγγραφής/ανάγνωσης.
        unsigned short f_flags; //Flags O_NONBLOCK κ.α.
        struct file_operations *f_op; //Σύνδεση με το
        //file operations του driver. Το κάνει ήδη.
        void *private_data; //Pointer για δεδομένα μας
        //που θα δείχνει σε δομή 
        //lunix_chrdev_state_struct
    };
```
Κρατάμε ξεχωριστό
``` C
	//enum lunix_msr_enum { BATT = 0, TEMP, LIGHT, N_LUNIX_MSR };

    /*
     * Private state for an open character device node
     */
struct lunix_chrdev_state_struct {
	enum lunix_msr_enum type; //Τι μέτρηση είναι
	struct lunix_sensor_struct *sensor; //Pointer
    //στον sensora που αφορά η μέτρηση

	/* A buffer used to hold cached textual info */
	int buf_lim;
	unsigned char buf_data[LUNIX_CHRDEV_BUFSZ];
	uint32_t buf_timestamp;
    
    //εσωτερικό lock.
	struct semaphore lock;

	/*
	 * Fixme: Any mode settings? e.g. blocking vs. non-blocking
	 */
};
```
Για κάθε ανοιχτό αρχείο κάνουμε allocate ένα ξεχωριστό <i>lunix_chrdev_state_struct</i> και αρχικοποιούμε τα πεδία του.


---
## read()

επιστρέφει έναν αριθμό από bytes
που αναπαριστούν την πιο πρόσφατη τιμή του μετρούμενου 
μεγέθους, σε αναγνώσιμη μορφή, μορφοποιημένη 
ως δεκαδικό αριθμό <i>(xx.yyy)</i>


Αν δεν έχουν ακόμη
παραληφθεί δεδομένα για τη συγκεκριμένη μέτρηση, ή αν γίνουν επόμενες
προσπάθειες ανάγνωσης, ο οδηγός κοιμίζει 
τη διεργασία έως ότου παραληφθούν νέα δεδομένα.

### Μπλοκάρισμα
Όταν δεν έχουν ληφθεί δεδομένα, 
ο οδηγός πρέπει να κοιμίζει τη διεργασία που τον κάλεσε.
Αυτή μετακινείται σε χωριστή ουρά διεργασιών, 
ανάλογα με τον αισθητήρα απ’ όπου περιμένει να φτάσουν δεδομένα.

Κάθε sensor struct έχει ένα πεδίο 
``` C
   	wait_queue_head_t wq;
```
Όπου κρατώνται οι διεργασίες που ζητούν ανάγνωση και δεν 
υπάρχουν δεδομένα, και άρα κοιμούνται με την <b>wait_event_interruptible()</b>.
Όταν τα δεδομένα είναι έτοιμα, αυτά αντιγράφονται στον χώρο χρήστη.


---
## close/release()
Deallocate memory που αφορά την <i>lunix_chrdev_state_struct</i> του κάθε ανοιχτού αρχείου.

---
## Δομή
Το lunix_line_desciple παίρνει raw data από τους αισθητήρες και τα βάζει
σε έναν buffer και καλεί τις συναρτήσεις στο lunix_protocol όπου γίνεται 
parsing των δεδομένων από την φόρμα με την οποία μεταφέρονται.
Μετά καλεί μία συνάρτηση στο lunix-sensors.c που διαχειρίζεται την
μεταφορά των parsed δεδομένων στις structures που υπάρχουν για τον κάθε
αισθητήτα 
``` C
    struct lunix_sensor_struct { //1 gia ka9e sensora
	/*
	 * A number of pages, one for each measurement.
	 * They can be mapped to userspace.
	 */
	struct lunix_msr_data_struct *msr_data[N_LUNIX_MSR];

	/*
	 * Spinlock used to assert mutual exclusion between
	 * the serial line discipline and the character device driver
	 */
	spinlock_t lock;

	/*
	 * A list of processes waiting to be woken up
	 * when this sensor has been updated with new data
	 */
	wait_queue_head_t wq;
};

```

Στo 
``` C 
    struct lunix_msr_data_struct *msr_data[];
```
υπάρχουν τα δεδομένα των μετρήσεων στην παρακάτω μορφή :
``` C
    struct lunix_msr_data_struct {
	    uint32_t magic;
	    uint32_t last_update;
	    uint32_t values[];
};
```

Τα δεδομένα αυτά ανανεώνονται με την χρήση ενός spinlock_t που ορίζεται
για κάθε αισθητήρα ώστε να μην υπάρχει διένεξη με το ανώτερο επίπεδο 
του <i>lunix_chrdev.c</i>.

 Όταν θέλουμε να διαβάσουμε δεδομένα και έχει ήδη το 
line_discpl το lock, βάζουμε την διεργασία σε ύπνο στο wait_queue_head_t του κάθε αισθητήρα μέχρι να ξεκλειδώσει 
<b>(εδώ αποκλείονται με <i>spinlock</i> όσες είναι για τον ίδιο
αισθητήρα.)
</b>

Παίρνουμε τα δεδομένα από τον sensor και τα βάζουμε
στο αντίστοιχο lunix_chrdev_state_struct όπου πλέον τα 
δεδομένα διακρίνονται με βάση την μέτρηση.

---
### Semaphores (Για δική μας χρήση)
``` C
struct semaphore. 
```
Αρχικοποιούνται
με χρήση της sema_init(), ενώ η πράξη P, κλείδωμα του σηματοφορέα υλοποιείται
από τη συνάρτηση down() και η πράξη V, απελευθέρωση του σηματοφορέα, από τη
συνάρτηση up()

Αντί της down() καλό είναι να χρησιμοποιείτε, αν δεν
υπάρχει ιδιαίτερος λόγος, τη συνάρτηση down_interruptible() στον κώδικά σας.
Η διαφορά της είναι ότι η διεργασία ξυπνά όχι μόνο αν κάποιος ξεκλειδώσει το
σηματοφορέα, αλλά και αν δεχθεί κάποιο σήμα. Έτσι, δεν εμφανίζεται κολλημένη για
πάντα και μπορεί να δεχθεί σήματα, πχ. το σήμα SIGINT πατώντας Ctrl-C




