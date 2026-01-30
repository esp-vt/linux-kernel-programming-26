## 5.2 Kernel Macro Challenge

**container_of:**

- Definition location: include/linux/container_of.h:19

- One-sentence explanation: <your explanation>

- Usage in block/: <file>:<line>

```
block/disk-events.c:310:	struct disk_events *ev = container_of(dwork, struct disk_events, dwork);
block/blk-sysfs.c:782:	struct gendisk *disk = container_of(kobj, struct gendisk, queue_kobj);
block/blk-sysfs.c:796:	struct gendisk *disk = container_of(kobj, struct gendisk, queue_kobj);
block/blk-sysfs.c:818:#define to_queue(atr) container_of((atr), struct queue_sysfs_entry, attr)
block/blk-sysfs.c:824:	struct gendisk *disk = container_of(kobj, struct gendisk, queue_kobj);
block/blk-sysfs.c:846:	struct gendisk *disk = container_of(kobj, struct gendisk, queue_kobj);
block/bio.c:376:	struct bio_set *bs = container_of(work, struct bio_set, rescue_work);
block/blk-stat.c:79:	struct blk_stat_callback *cb = timer_container_of(cb, t, timer);
block/blk-stat.c:172:	cb = container_of(head, struct blk_stat_callback, rcu);
block/blk-ioprio.c:64:	return container_of(blkcg_to_cpd(blkcg, &ioprio_policy),
block/blk-ioprio.c:111:	struct ioprio_blkcg *blkcg = container_of(cpd, typeof(*blkcg), cpd);
block/blk-iocost.c:662:	return container_of(rqos, struct ioc, rqos);
block/blk-iocost.c:681:	return pd ? container_of(pd, struct ioc_gq, pd) : NULL;
block/blk-iocost.c:696:	return container_of(blkcg_to_cpd(blkcg, &blkcg_policy_iocost),
block/blk-iocost.c:1471:	struct iocg_wait *wait = container_of(wq_entry, struct iocg_wait, wait);
block/blk-iocost.c:1585:	struct ioc_gq *iocg = container_of(timer, struct ioc_gq, waitq_timer);
block/blk-iocost.c:2245:	struct ioc *ioc = container_of(timer, struct ioc, timer);
block/blk-iocost.c:2961:	kfree(container_of(cpd, struct ioc_cgrp, cpd));
block/.bdev.o.cmd:376:  include/linux/container_of.h \
block/blk-cgroup.h:124:	return css ? container_of(css, struct blkcg, css) : NULL;
block/blk-ia-ranges.c:54:		container_of(attr, struct blk_ia_range_sysfs_entry, attr);
block/blk-ia-ranges.c:56:		container_of(kobj, struct blk_independent_access_range, kobj);
block/blk-ia-ranges.c:92:		container_of(kobj, struct blk_independent_access_ranges, kobj);
block/.disk-events.o.d:24: include/linux/container_of.h include/linux/bitops.h include/linux/bits.h \
block/blk-cgroup.c:115:	struct blkcg_gq *blkg = container_of(work, struct blkcg_gq,
block/blk-cgroup.c:165:	struct blkcg_gq *blkg = container_of(rcu, struct blkcg_gq, rcu_head);
block/blk-cgroup.c:196:	struct blkcg_gq *blkg = container_of(ref, struct blkcg_gq, refcnt);
block/blk-cgroup.c:206:	struct blkcg_gq *blkg = container_of(work, struct blkcg_gq,
block/.disk-events.o.cmd:115:  include/linux/container_of.h \
block/blk-mq-sched.c:50:	struct request *rqa = container_of(a, struct request, queuelist);
block/blk-mq-sched.c:51:	struct request *rqb = container_of(b, struct request, queuelist);
block/genhd.c:1238:	struct device *dev = container_of(kobj, typeof(*dev), kobj);
block/.bdev.o.d:70: include/linux/container_of.h include/linux/bitops.h \
block/bdev.c:44:	return container_of(inode, struct bdev_inode, vfs_inode);
block/bdev.c:49:	return &container_of(bdev, struct bdev_inode, bdev)->vfs_inode;
block/.blk-mq.o.d:23: include/linux/container_of.h include/linux/build_bug.h \
block/bsg.c:35:	return container_of(inode->i_cdev, struct bsg_device, cdev);
block/bsg.c:172:	struct bsg_device *bd = container_of(dev, struct bsg_device, device);
block/.bsg.o.d:5: include/linux/container_of.h include/linux/build_bug.h \
block/.bsg-lib.o.d:73: include/linux/container_of.h include/linux/bitops.h \
block/blk-wbt.c:98:	return container_of(rqos, struct rq_wb, rqos);
block/blk-mq-tag.c:587:	struct blk_mq_tags *tags = container_of(head, struct blk_mq_tags,
block/blk-core.c:250:	struct request_queue *q = container_of(rcu_head,
block/blk-core.c:377:		container_of(ref, struct request_queue, q_usage_counter);
block/blk-core.c:384:	struct request_queue *q = timer_container_of(q, t, timeout);
block/.blk-mq-dma.o.cmd:219:  include/linux/container_of.h \
block/bfq-iosched.c:448:	return container_of(icq, struct bfq_io_cq, icq);
block/bfq-iosched.c:862:		container_of(
block/bfq-iosched.c:924:		struct bfq_weight_counter *__counter = container_of(*new,
block/bfq-iosched.c:2504:	    blk_rq_pos(container_of(rb_prev(&req->rb_node),
block/bfq-iosched.c:4894:			container_of(bfqq->woken_list.first,
block/bfq-iosched.c:7055:	struct bfq_data *bfqd = container_of(timer, struct bfq_data,
block/blk-ioc.c:107:	struct io_context *ioc = container_of(work, struct io_context,
block/.blk-mq-sched.o.d:23: include/linux/container_of.h include/linux/build_bug.h \
block/blk-crypto-fallback.c:376:		container_of(work, struct bio_fallback_crypt_ctx, work);
block/blk-mq.c:1552:		container_of(work, struct request_queue, requeue_work.work);
block/blk-mq.c:1719:		container_of(work, struct request_queue, timeout_work);
block/blk-mq.c:1878:	hctx = container_of(wait, struct blk_mq_hw_ctx, dispatch_wait);
block/blk-mq.c:2541:		container_of(work, struct blk_mq_hw_ctx, run_work.work);
block/blk-mq.c:3910:		container_of(head, struct blk_flush_queue, rcu_head);
block/blk-rq-qos.c:208:	struct rq_qos_wait_data *data = container_of(curr,
block/blk-mq-sysfs.c:18:	struct blk_mq_ctxs *ctxs = container_of(kobj, struct blk_mq_ctxs, kobj);
block/blk-mq-sysfs.c:26:	struct blk_mq_ctx *ctx = container_of(kobj, struct blk_mq_ctx, kobj);
block/blk-mq-sysfs.c:34:	struct blk_mq_hw_ctx *hctx = container_of(kobj, struct blk_mq_hw_ctx,
block/blk-mq-sysfs.c:56:	entry = container_of(attr, struct blk_mq_hw_ctx_sysfs_entry, attr);
block/blk-mq-sysfs.c:57:	hctx = container_of(kobj, struct blk_mq_hw_ctx, kobj);
block/elevator.c:159:	e = container_of(kobj, struct elevator_queue, kobj);
block/elevator.c:420:#define to_elv(atr) container_of_const((atr), struct elv_fs_entry, attr)
block/elevator.c:432:	e = container_of(kobj, struct elevator_queue, kobj);
block/elevator.c:451:	e = container_of(kobj, struct elevator_queue, kobj);
block/blk-iolatency.c:105:	return container_of(rqos, struct blk_iolatency, rqos);
block/blk-iolatency.c:185:	return pd ? container_of(pd, struct iolatency_grp, pd) : NULL;
block/blk-iolatency.c:653:	struct blk_iolatency *blkiolat = timer_container_of(blkiolat, t,
block/blk-iolatency.c:730:	struct blk_iolatency *blkiolat = container_of(work, struct blk_iolatency,
block/fops.c:191:	dio = container_of(bio, struct blkdev_dio, bio);
block/fops.c:298:	struct blkdev_dio *dio = container_of(bio, struct blkdev_dio, bio);
block/fops.c:340:	dio = container_of(bio, struct blkdev_dio, bio);
block/blk-crypto-sysfs.c:26:	return container_of(kobj, struct blk_crypto_kobj, kobj)->profile;
block/blk-crypto-sysfs.c:31:	return container_of(attr, struct blk_crypto_attr, attr);
block/blk-crypto-sysfs.c:151:	kfree(container_of(kobj, struct blk_crypto_kobj, kobj));
block/bfq-wf2q.c:158:	bfqg = container_of(group_sd, struct bfq_group, sched_data);
block/bfq-wf2q.c:201:	bfqg = container_of(entity, struct bfq_group, entity);
block/bfq-wf2q.c:224:	struct bfq_group *bfqg = container_of(sd, struct bfq_group, sched_data);
block/bfq-wf2q.c:233:	struct bfq_group *bfqg = container_of(sd, struct bfq_group, sched_data);
block/bfq-wf2q.c:275:		bfqq = container_of(entity, struct bfq_queue, entity);
block/kyber-iosched.c:276:	struct kyber_queue_data *kqd = timer_container_of(kqd, t, timer);
block/kyber-iosched.c:686:	struct sbq_wait *wait = container_of(wqe, struct sbq_wait, wait);
block/blk-throttle.c:1128:	struct throtl_service_queue *sq = timer_container_o (sq, t,
block/blk-throttle.c:1207:	struct throtl_data *td = container_of(work, struct throtl_data,
block/bio-integrity-auto.c:39:		container_of(work, struct bio_integrity_data, work);
block/bio-integrity-auto.c:83:		container_of(bip, struct bio_integrity_data, bip);
block/bsg-lib.c:158:	struct bsg_job *job = container_of(kref, struct bsg_job, kref);
block/bsg-lib.c:279:		container_of(q->tag_set, struct bsg_set, tag_set);
block/bsg-lib.c:324:			container_of(q->tag_set, struct bsg_set, tag_set);
block/bsg-lib.c:338:		container_of(rq->q->tag_set, struct bsg_set, tag_set);
block/bfq-cgroup.c:276:	return pd ? container_of(pd, struct bfq_group, pd) : NULL;
block/bfq-cgroup.c:307:	return group_entity ? container_of(group_entity, struct bfq_group,
block/bfq-cgroup.c:485:	return cpd ? container_of(cpd, struct bfq_group_data, pd) : NULL;
block/bfq-cgroup.c:588:		struct bfq_group *curr_bfqg = container_of(entity,
```

- Explanation of usage: <your explanation>

**READ_ONCE / WRITE_ONCE:**

- Definition location: include/asm-generic/rwonce.h:47

- Why they exist: To prevent the compiler from performing optimizations like reordering or load tearing, ensuring that the variable is accessed exactly once as intended in concurrent programming


**current:**

- Definition location: arch/x86/include/asm/current.h:28

- Mechanism: It uses architecture-specific per-cpu variables (accessed via the GS segment register on x86) to efficiently retrieve the pointer to the task_struct of the currently running process
