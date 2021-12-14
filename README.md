# oslab2



## Parameter
Το lunix εχει parameter :
 ``` C
  int lunix_sensor_cnt
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
## read()

επιστρέφει έναν αριθμό από bytes
που αναπαριστούν την πιο πρόσφατη τιμή του μετρούμενου 
μεγέθους, σε αναγνώσιμη μορφή, μορφοποιημένη 
ως δεκαδικό αριθμό


Αν δεν έχουν ακόμη
παραληφθεί δεδομένα για τη συγκεκριμένη μέτρηση, ή αν γίνουν επόμενες
προσπάθειες ανάγνωσης, ο οδηγός θα πρέπει να κοιμίζει 
τη διεργασία (βλ. 7.3) έως ότου παραληφθούν νέα δεδομένα.

### Μπλοκάρισμα
Όταν δεν έχουν ληφθεί δεδομένα, 
ο οδηγός πρέπει να κοιμίζει τη διεργασία: αυτό σημαίνει ότι
η διεργασία αλλάζει κατάσταση, δεν είναι πλέον «έτοιμη» προς εκτέλεση και ο
χρονοδρομολογητής θα διαλέξει κάποια άλλη να εκτελεστεί στη θέση της. Ενώ είναι
μπλοκαρισμένη, η διεργασία πρέπει να μετακινείται σε χωριστή ουρά διεργασιών,
ανάλογα με τον αισθητήρα απ’ όπου περιμένει να φτάσουν δεδομένα.

Κάθε sensor struct έχει ένα πεδίο 
``` C
   	wait_queue_head_t wq;
```
Όπου θα κρατώνται οι διεργασίες που ζητούν ανάγνωση και δεν 
υπάρχουν δεδομένα, και άρα κοιμούνται.

<b>
Όταν η read() διαπιστώσει ότι δεν υπάρχουν νέα δεδομένα και η διαδικασία πρέπει
να κοιμηθεί, χρησιμοποιεί την κλήση wait_event_interruptible() ώστε η
τρέχουσα διεργασία να μπλοκάρει στην ανάλογη ουρά. σελ 153.
</b>

---
## open()
``` C
    int open (struct inode *, struct file *)
```

Θα χρησιμοποίησουμε 
``` C
    unsigned int iminor(struct inode *inode);
    unsigned int imajor(struct inode *inode);
```
ώστε να μπορούμε να ανακτήσουμε τον major και minor number του αρχείου, ώστε το ανοιχτό
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
Πρέπει για κάθε αρχείο να κρατάμε ξεχωριστό
``` C
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
---
## close/release()
Deallocate memory

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
για κάθε αισθητήρα.

(Εικασία : Όταν θέλουμε να διαβάσουμε δεδομένα και έχει ήδη το 
line_discpl το lock, βάζουμε την διεργασία σε ύπνο στο wait_queue_head_t του κάθε αισθητήρα μέχρι να ξεκλειδώσει 
<b>(εδώ άλλες είναι για τον ίδιο
αισθητήρα και άλλες για την ίδια μέτρηση του κάθε αισθητήτα. Στην πρώτη περίπτωση τους κλειδώνει όλους όσους είναι για τον ίδιο αισθητήρα.)
</b>
)

Θα πάρουμε τα δεδομένα από τον sensor και θα τα βάλουμε 
στο αντίστοιχο lunix_chrdev_state_struct.


### Semaphores
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